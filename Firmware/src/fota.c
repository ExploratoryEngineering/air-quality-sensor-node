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

#define BLOCK_SIZE COAP_BLOCK_256
#define BLOCK_BYTES 256
#define MAX_BUFFER_LEN 512

extern char FOTA_COAP_SERVER[NVS_APN_COUNT][CONFIG_NAME_SIZE];
extern int FOTA_COAP_PORT[NVS_APN_COUNT];
extern char FOTA_COAP_REPORT_PATH[NVS_APN_COUNT][CONFIG_NAME_SIZE];
extern int ACTIVE_APN_INDEX;

// This is the general buffer used by the FOTA and flash writing functions
static uint8_t request_buffer[MAX_BUFFER_LEN];
static uint8_t response_buffer[MAX_BUFFER_LEN];

static void do_reboot()
{
	LOG_INF("Rebooting after update");
	int ret = boot_request_upgrade(BOOT_UPGRADE_TEST);
	if (ret)
	{
		LOG_ERR("Error attempting reboot: %d", ret);
	}

	sys_reboot(0);
}

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
			LOG_ERR("Error confirming image: %d", ret);
			return ret;
		}
		LOG_INF("New image confirmed");
	}
	return 0;
}

static int fota_encode_simple_report(uint8_t *buffer, size_t *len)
{
	size_t sz = encode_tlv_string(buffer, FIRMWARE_VER_ID, CLIENT_FIRMWARE_VER);
	sz += encode_tlv_string(buffer + sz, CLIENT_MANUFACTURER_ID, CLIENT_MANUFACTURER);
	sz += encode_tlv_string(buffer + sz, SERIAL_NUMBER_ID, CLIENT_SERIAL_NUMBER);
	sz += encode_tlv_string(buffer + sz, MODEL_NUMBER_ID, CLIENT_MODEL_NUMBER);
	*len = sz;
	return 0;
}

static int fota_decode_simple_response(simple_fota_response_t *resp, const uint8_t *buf, size_t len)
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

/*static */int fota_report_version(simple_fota_response_t *resp)
{
	struct coap_packet p;
	int err = 0;
#define TOKEN_SIZE 8
	char token[TOKEN_SIZE];
	sys_csrand_get(token, TOKEN_SIZE);

	if (coap_packet_init(&p, request_buffer, sizeof(request_buffer), 1, COAP_TYPE_CON,
						 TOKEN_SIZE, token, COAP_METHOD_POST,
						 coap_next_id()) < 0)
	{
		LOG_ERR("Unable to iniitialize CoAP packet");
		return -1;
	}
	if (coap_packet_append_option(&p, COAP_OPTION_URI_PATH,
								  FOTA_COAP_REPORT_PATH[ACTIVE_APN_INDEX],
								  strlen(FOTA_COAP_REPORT_PATH[ACTIVE_APN_INDEX])) < 0)
	{
		LOG_ERR("Could not append path option to packet");
		return -1;
	}
	if (coap_packet_append_payload_marker(&p) < 0)
	{
		LOG_ERR("Unable to append payload marker to packet");
		return -1;
	}

	uint8_t tmpBuf[256];
	size_t len;
	if (fota_encode_simple_report(tmpBuf, &len) < 0)
	{
		LOG_ERR("Unable to encode FOTA report");
		return -1;
	}
	if (coap_packet_append_payload(&p, tmpBuf, len) < 0)
	{
		LOG_ERR("Unable to append payload to CoAP packet");
		return -1;
	}

	LOG_DBG("Sending %d bytes to %s:%d", p.offset, log_strdup(FOTA_COAP_SERVER[ACTIVE_APN_INDEX]), FOTA_COAP_PORT[ACTIVE_APN_INDEX]);

	int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (sock < 0)
	{
		LOG_ERR("Error opening socket: %d", sock);
		return sock;
	}

	struct sockaddr_in remote_addr = {
		sin_family : AF_INET,
	};
	remote_addr.sin_port = htons(FOTA_COAP_PORT[ACTIVE_APN_INDEX]);
	net_addr_pton(AF_INET, FOTA_COAP_SERVER[ACTIVE_APN_INDEX], &remote_addr.sin_addr);

	err = sendto(sock, request_buffer, p.offset, 0, (const struct sockaddr *)&remote_addr, sizeof(remote_addr));
	if (err < 0)
	{
		LOG_ERR("Unable to send CoAP packet: %d", err);
		close(sock);
		return err;
	}
	LOG_DBG("Sent %d bytes, waiting for response", p.offset);

	int received = 0;

	if (!wait_for_response(sock))
	{
		close(sock);
		return -1;
	}
	received = recvfrom(sock, response_buffer, sizeof(response_buffer), 0, NULL, NULL);
	if (received < 0)
	{
		LOG_ERR("Error receiving data: %d", received);
		close(sock);
		return received;
	}
	close(sock);

	struct coap_packet reply;

	if (coap_packet_parse(&reply, response_buffer, received, NULL, 0) < 0)
	{
		LOG_ERR("Could not parse CoAP reply from server");
		return -1;
	}

	uint16_t payload_len = 0;
	const uint8_t *buf = coap_packet_get_payload(&reply, &payload_len);

	LOG_DBG("Decode response (%d bytes)", len);
	if (fota_decode_simple_response(resp, buf, payload_len) < 0)
	{
		LOG_ERR("Could not decode response from server");
		return -1;
	}

	return 0;
}

static bool fota_download_image(simple_fota_response_t *resp)
{
	LOG_INF("Downloading new firmware image...");
	struct coap_block_context block_ctx;
	memset(&block_ctx, 0, sizeof(block_ctx));

	coap_block_transfer_init(&block_ctx, BLOCK_SIZE, 0);

	int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (sock < 0)
	{
		LOG_ERR("Error opening socket: %d", sock);
		return false;
	}

	struct sockaddr_in remote_addr = {
		sin_family : AF_INET,
	};
	remote_addr.sin_port = htons(resp->port);
	net_addr_pton(AF_INET, resp->host, &remote_addr.sin_addr);
	if (connect(sock, (struct sockaddr *)&remote_addr, sizeof(remote_addr)) < 0)
	{
		LOG_ERR("Unable to connect to %s:%d", log_strdup(resp->host), resp->port);
		close(sock);
		return false;
	}

	bool last_block = false;
	const uint8_t token_length = 8;
	uint8_t token[token_length];
	size_t total_size = 0;
	memcpy(token, coap_next_token(), token_length);
	while (!last_block)
	{
		struct coap_packet request;
		memset(request_buffer, 0, MAX_BUFFER_LEN);
		if (coap_packet_init(&request, request_buffer, MAX_BUFFER_LEN, 1,
							 COAP_TYPE_CON, token_length, token,
							 COAP_METHOD_GET, coap_next_id()) < 0)
		{
			LOG_ERR("Could not init request packet");
			close(sock);
			return false;
		}

		// Assuming a single path entry here. It might be more.
		if (coap_packet_append_option(&request, COAP_OPTION_URI_PATH,
									  resp->path, strlen(resp->path)) < 0)
		{
			LOG_ERR("Could not init path option");
			close(sock);
			return false;
		}
		if (coap_append_block2_option(&request, &block_ctx) < 0)
		{
			LOG_ERR("Could not append block option");
			close(sock);
			return false;
		}
		int sent = send(sock, request.data, request.offset, 0);
		if (sent != request.offset)
		{
			close(sock);
			LOG_ERR("Error sending %d bytes on socket", request.offset);
			return false;
		}

		if (!wait_for_response(sock))
		{
			close(sock);
			return false;
		}

		memset(response_buffer, 0, MAX_BUFFER_LEN);
		int received = recv(sock, response_buffer, MAX_BUFFER_LEN, MSG_DONTWAIT);
		if (received <= 0)
		{
			LOG_ERR("Error receiving data: %d", received);
			close(sock);
			return false;
		}

		struct coap_packet reply;

		if (coap_packet_parse(&reply, response_buffer, received, NULL, 0) < 0)
		{
			LOG_ERR("Invalid data received");
			close(sock);
			return false;
		}

		if (coap_update_from_block(&reply, &block_ctx) < 0)
		{
			LOG_ERR("Error updating from block");
			close(sock);
			return false;
		}
		last_block = !coap_next_block(&reply, &block_ctx);
		uint16_t payload_len = 0;
		const uint8_t *payload = coap_packet_get_payload(&reply, &payload_len);
		total_size += payload_len;
		LOG_DBG("Retreived block %d (%d of %d bytes) ", block_ctx.current / BLOCK_BYTES, total_size, block_ctx.total_size);

		bool first = total_size == payload_len;
		bool last = total_size == block_ctx.total_size;
		if (write_firmware_block(payload, payload_len, first, last, block_ctx.total_size) < 0)
		{
			LOG_ERR("Error writing firmware block");
			close(sock);
			return false;
		}
	}
	close(sock);
	return true;
}

bool fota_init()
{
	LOG_INF("Firmware version: %s", CLIENT_FIRMWARE_VER);
	LOG_INF("Model number:     %s", CLIENT_MODEL_NUMBER);
	LOG_INF("Serial numbera:   %s", CLIENT_SERIAL_NUMBER);
	LOG_INF("Manufacturer:     %s", CLIENT_MANUFACTURER);

	int ret = init_image();
	if (ret)
	{
		LOG_ERR("Error initialising image: %d", ret);
		return false;
	}
	return true;
}

bool fota_run()
{
	// Ping home
	simple_fota_response_t resp;
	int ret = fota_report_version(&resp);
	if (ret)
	{
		LOG_ERR("Error reporting version: %d", ret);
		return false;
	}
	LOG_INF("Endpoint: coap://%s:%d/%s", log_strdup(resp.host), resp.port, log_strdup(resp.path));
	if (resp.scheduled_update)
	{
		LOG_INF("Update is scheduled for this device");
		ret = fota_download_image(&resp);
		if (!ret)
		{
			LOG_ERR("Unable to download image: %d", ret);
			return false;
		}
		do_reboot();
		return true;
	}
	return false;
}

bool wait_for_response(int sock)
{
	// Poll for response.
	struct pollfd poll_set[1];
	poll_set[0].fd = sock;
	poll_set[0].events = POLLIN;

	bool data = false;
	int wait = 0;
	const int poll_wait_ms = 500;
	const int max_wait_time_ms = 60000;
	while (!data)
	{
		poll(poll_set, 1, poll_wait_ms);
		if ((poll_set[0].revents & POLLIN) == POLLIN)
		{
			data = true;
			continue;
		}
		wait++;
		if (wait > (max_wait_time_ms / poll_wait_ms))
		{
			LOG_ERR("Server response timed out");
			return false;
		}
	}
	return true;
}