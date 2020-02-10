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

// This is the general buffer used by the FOTA and flash writing functions
static uint8_t buffer[256];
#define PHONE_HOME_PATH "d"

#define CHECK_ERR(x)                   \
	err = (x);                         \
	if (err)                           \
	{                                  \
		LOG_ERR("Error exec:%d", err); \
		return err;                    \
	}

#define FOTA_COAP_SERVER "172.16.7.197"
#define FOTA_COAP_PORT 6683

static int fota_report_version()
{
	struct coap_packet p;
	int err = 0;
#define TOKEN_SIZE 8
	char token[TOKEN_SIZE];
	sys_csrand_get(token, TOKEN_SIZE);

	CHECK_ERR(coap_packet_init(&p, buffer, sizeof(buffer), 1, COAP_TYPE_CON, TOKEN_SIZE, token, COAP_METHOD_POST, coap_next_id()));
	CHECK_ERR(coap_packet_append_option(&p, COAP_OPTION_URI_PATH, PHONE_HOME_PATH, strlen(PHONE_HOME_PATH)));
	CHECK_ERR(coap_packet_append_payload_marker(&p));

	uint8_t tmpBuf[64];

	int len = encode_tlv_string(tmpBuf, FIRMWARE_VER_ID, CLIENT_FIRMWARE_VER);
	CHECK_ERR(coap_packet_append_payload(&p, tmpBuf, len));

	len = encode_tlv_string(tmpBuf, CLIENT_MANUFACTURER_ID, CLIENT_MANUFACTURER);
	CHECK_ERR(coap_packet_append_payload(&p, tmpBuf, len));

	len = encode_tlv_string(tmpBuf, SERIAL_NUMBER_ID, CLIENT_SERIAL_NUMBER);
	CHECK_ERR(coap_packet_append_payload(&p, tmpBuf, len));

	len = encode_tlv_string(tmpBuf, MODEL_NUMBER_ID, "2 holy cow");
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

	err = sendto(sock, buffer, p.offset, 0, (const struct sockaddr *)&remote_addr, sizeof(remote_addr));
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
		received = recvfrom(sock, buffer, sizeof(buffer), 0, NULL, NULL);
		if (received < 0)
		{
			k_sleep(K_MSEC(2000));
			LOG_ERR("Error receiving data: %d", received);
			close(sock);
			return received;
		}
		if (received == 0)
		{
			k_sleep(K_MSEC(500));
		}
	}
	k_sleep(K_MSEC(2000));
	close(sock);

	LOG_DBG("Got response (%d) bytes", received);
	CHECK_ERR(coap_packet_parse(&p, buffer, received, NULL, 0));

	LOG_INF("Successfully received a response from FOTA CoAP server");
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
	ret = fota_report_version();
	if (ret)
	{
		LOG_ERR("Error reporting version: %d", ret);
	}
	return ret;
}
