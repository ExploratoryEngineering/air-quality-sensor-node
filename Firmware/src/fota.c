#include <zephyr.h>
#include <logging/log.h>

#include <dfu/mcuboot.h>

#include <misc/reboot.h>
#include <stdio.h>
#include <net/coap.h>
#include <net/socket.h>

#include "fota.h"
#include "fota_tlv.h"
#include "flash.h"

LOG_MODULE_REGISTER(FOTA, CONFIG_FOTA_LOG_LEVEL);

static struct k_sem fota_sem;

/*static void do_reboot()
{
	int ret = boot_request_upgrade(BOOT_UPGRADE_TEST);
	if (ret)
	{
		LOG_ERR("boot_request_upgrade: %d", ret);
	}

	sys_reboot(0);
}*/

static int init_image()
{
	// This might need some additional checks. If the update has failed
	// we'll be booting into an confirmed image but this might also be for
	// other reasons. Store the current state somewhere in flash before making
	// assumptions on the state. For now we'll just confirm the image.
	if (!boot_is_img_confirmed())
	{
		int ret = boot_write_img_confirmed();
		if (ret)
		{
			LOG_ERR("confirm image: %d", ret);
			return ret;
		}
		LOG_INF("Firmware update succeeded");
	}
	return 0;
}

void fota_disable()
{
	LOG_DBG("FOTA reboots disabled");
	k_sem_take(&fota_sem, K_FOREVER);
}

void fota_enable()
{
	LOG_DBG("FOTA reboots enabled");
	k_sem_give(&fota_sem);
}


#define MAX_BUFFER_LEN 512
// This is the general buffer used by the FOTA and flash writing functions
static uint8_t request_buffer[MAX_BUFFER_LEN];
static uint8_t response_buffer[MAX_BUFFER_LEN];

#define CHECK_ERR(x)                   \
	err = (x);                         \
	if (err)                           \
	{                                  \
		LOG_ERR("Error exec:%d", err); \
		return err;                    \
	}

#define FOTA_COAP_SERVER "172.16.15.14"
#define FOTA_COAP_PORT 6683
#define FOTA_COAP_REPORT_PATH "u"

int fota_encode_simple_report(uint8_t *buffer, size_t *len) {
	size_t sz = encode_tlv_string(buffer, FIRMWARE_VER_ID, CLIENT_FIRMWARE_VER);
	sz += encode_tlv_string(buffer + sz, CLIENT_MANUFACTURER_ID, CLIENT_MANUFACTURER);
	sz += encode_tlv_string(buffer + sz, SERIAL_NUMBER_ID, CLIENT_SERIAL_NUMBER);
	sz += encode_tlv_string(buffer + sz, MODEL_NUMBER_ID, CLIENT_MODEL_NUMBER);
	*len = sz;
	return 0;
}

int fota_report_version(simple_fota_response_t *resp)
{
	struct coap_packet p;
	int err = 0;
#define TOKEN_SIZE 8
	char token[TOKEN_SIZE];
	sys_csrand_get(token, TOKEN_SIZE);

	CHECK_ERR(coap_packet_init(&p, request_buffer, sizeof(request_buffer), 1, COAP_TYPE_CON,
							   TOKEN_SIZE, token, COAP_METHOD_POST,
							   coap_next_id()));
	CHECK_ERR(coap_packet_append_option(&p, COAP_OPTION_URI_PATH,
										FOTA_COAP_REPORT_PATH,
										strlen(FOTA_COAP_REPORT_PATH)));
	CHECK_ERR(coap_packet_append_payload_marker(&p));

	uint8_t tmpBuf[256];
	size_t len;
	CHECK_ERR(fota_encode_simple_report(tmpBuf, &len));
	CHECK_ERR(coap_packet_append_payload(&p, tmpBuf, len));

	LOG_INF("Sending %d bytes to %s:%d", p.offset, log_strdup(FOTA_COAP_SERVER), FOTA_COAP_PORT);

	int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (sock < 0)
	{
		LOG_ERR("Error opening socket: %d", sock);
		return sock;
	}

	struct sockaddr_in remote_addr = {
		sin_family : AF_INET,
	};
	remote_addr.sin_port = htons(FOTA_COAP_PORT);
	net_addr_pton(AF_INET, FOTA_COAP_SERVER, &remote_addr.sin_addr);

	err = sendto(sock, request_buffer, p.offset, 0, (const struct sockaddr *)&remote_addr, sizeof(remote_addr));
	if (err < 0)
	{
		LOG_ERR("Unable to send CoAP packet: %d", err);
		close(sock);
		return err;
	}
	LOG_DBG("Sent %d bytes, waiting for response", p.offset);

	int received = 0;

	while (received == 0)
	{
		received = recvfrom(sock, response_buffer, sizeof(response_buffer), 0, NULL, NULL);
		if (received < 0)
		{
			LOG_ERR("Error receiving data: %d", received);
			close(sock);
			return received;
		}
		if (received == 0)
		{
			k_sleep(K_MSEC(500));
		}
	}
	close(sock);

	LOG_DBG("Got response (%d) bytes", received);
	struct coap_packet reply;

	CHECK_ERR(coap_packet_parse(&reply, response_buffer, received, NULL, 0));

	LOG_INF("Successfully received a response from FOTA CoAP server");

	uint16_t payload_len = 0;
	const uint8_t *buf = coap_packet_get_payload(&reply, &payload_len);

	// Decode the response
	LOG_INF("Decode response (%d bytes)", len);
	CHECK_ERR(fota_decode_simple_response(resp, buf, payload_len));

	return 0;
}

int fota_init()
{
	LOG_DBG("Firmware version: %s", CLIENT_FIRMWARE_VER);
	LOG_DBG("Model number:     %s", CLIENT_MODEL_NUMBER);
	LOG_DBG("Serial numbera:   %s", CLIENT_SERIAL_NUMBER);
	LOG_DBG("Manufacturer:     %s", CLIENT_MANUFACTURER);

	k_sem_init(&fota_sem, 1, 1);

	int ret = init_image();
	if (ret)
	{
		LOG_ERR("Error initialising image: %d", ret);
		return ret;
	}

	// Ping home
	simple_fota_response_t resp;
	ret = fota_report_version(&resp);
	if (ret)
	{
		LOG_ERR("Error reporting version: %d", ret);
		return ret;
	}
	LOG_INF("Endpoint: coap://%s:%d/%s", log_strdup(resp.host), resp.port, log_strdup(resp.path));
	if (resp.scheduled_update)
	{
		LOG_INF("Update is scheduled for this device");
		ret = fota_download_image(&resp);
		if (ret) {
			LOG_ERR("Unable to download image: %d", ret);
		}
	}
	return ret;
}

int fota_decode_simple_response(simple_fota_response_t *resp, const uint8_t *buf, size_t len)
{
	size_t idx = 0;
	int err = 0;
	while (idx < len)
	{
		uint8_t id = buf[idx++];
		switch (id)
		{
		case HOST_ID:
			err = decode_tlv_string(buf, &idx, resp->host);
			if (err)
			{
				return err;
			}
			break;
		case PORT_ID:
			err = decode_tlv_uint32(buf, &idx, &resp->port);
			if (err)
			{
				return err;
			}
			break;
		case PATH_ID:
			err = decode_tlv_string(buf, &idx, resp->path);
			if (err)
			{
				return err;
			}
			break;
		case AVAILABLE_ID:
			err = decode_tlv_bool(buf, &idx, &resp->scheduled_update);
			if (err)
			{
				return err;
			}
			break;
		default:
			LOG_ERR("Unknown field id in FOTA response: %d", id);
			return -1;
		}
	}
	return 0;
}


#define BLOCK_SIZE COAP_BLOCK_128
#define BLOCK_BYTES 128

int fota_download_image(simple_fota_response_t *resp) {
	struct coap_block_context block_ctx;
	memset(&block_ctx, 0, sizeof(block_ctx));
	int err;
	coap_block_transfer_init(&block_ctx, BLOCK_SIZE, 0);

	// Send request
	int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (sock < 0)
	{
		LOG_ERR("Error opening socket: %d", sock);
		return sock;
	}

	struct sockaddr_in remote_addr = {
		sin_family : AF_INET,
	};
	remote_addr.sin_port = htons(resp->port);
	net_addr_pton(AF_INET, resp->host, &remote_addr.sin_addr);
	err = connect(sock, (struct sockaddr *)&remote_addr, sizeof(remote_addr));
	if (err) {
		LOG_ERR("Unable to connect to %s:%d", log_strdup(resp->host), resp->port);
		return err;
	}

	int max_limit = 4096/BLOCK_BYTES;
	int current_count = 0;
	bool last_block = false;
	while (!last_block) {
		current_count++;
		if (current_count > max_limit) {
			LOG_ERR("This is going nowhere. I'm terminating");
			close(sock);
			return -1;
		}
		LOG_INF("1 => Block transfer block %d (total: %d) ", block_ctx.current/BLOCK_BYTES, block_ctx.total_size);
		struct coap_packet request;
		memset(request_buffer, 0, MAX_BUFFER_LEN);
		if (coap_packet_init(&request, request_buffer, MAX_BUFFER_LEN, 1,
								COAP_TYPE_CON, 8, coap_next_token(),
								COAP_METHOD_GET, coap_next_id()))
		{
			LOG_ERR("Could not init request packet");
			close(sock);
			return -1;
		}

		LOG_INF("2 => Block transfer block %d (total: %d) ", block_ctx.current/BLOCK_BYTES, block_ctx.total_size);
		// Assuming a single path entry here. It might be more.
		if(coap_packet_append_option(&request, COAP_OPTION_URI_PATH,
											resp->path, strlen(resp->path)) < 0)
		{
			LOG_ERR("Could not init path option");
			close(sock);
			return -2;
		}
		if (coap_append_block2_option(&request, &block_ctx) < 0) {
			LOG_ERR("Could not append block option");
			close(sock);
			return -3;
		}
		LOG_INF("3 => Block transfer block %d (total: %d) ", block_ctx.current/BLOCK_BYTES, block_ctx.total_size);
		err = send(sock, request.data, request.offset, 0);
		if (err != request.offset) {
			close(sock);
			LOG_ERR("Error sending %d bytes on socket", request.offset);
		}

		memset(response_buffer, 0, MAX_BUFFER_LEN);
		int r = recv(sock, response_buffer, MAX_BUFFER_LEN, 0);
		if (r < 0) {
			LOG_ERR("Error receiving data: %d", r);
			close(sock);
			return r;
		}

		LOG_INF("Got %d bytes from server", r);
		struct coap_packet reply;

		if (coap_packet_parse(&reply, response_buffer, r, NULL, 0) < 0) {
			LOG_WRN("Invalid data received");
			close(sock);
			return -1;
		}
		LOG_INF("Pre ==> Block transfer block %d (total: %d) ", block_ctx.current/BLOCK_BYTES, block_ctx.total_size);

		if (coap_update_from_block(&reply, &block_ctx) < 0) {
			LOG_WRN("Error updating from block");
			close(sock);
			return -1;
		}
		LOG_INF("Post => Block transfer block %d (total: %d) ", block_ctx.current/BLOCK_BYTES, block_ctx.total_size);

		last_block = !coap_next_block(&reply, &block_ctx);
		LOG_INF("Post 2> Block transfer block %d (total: %d) ", block_ctx.current/BLOCK_BYTES, block_ctx.total_size);
//		uint16_t payload_len = 0;
//		coap_packet_get_payload(&reply, &payload_len);
	}
	close(sock);
	return 0;
}