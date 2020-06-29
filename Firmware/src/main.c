/*
**   Copyright 2020 Telenor Digital AS
**
**  Licensed under the Apache License, Version 2.0 (the "License");
**  you may not use this file except in compliance with the License.
**  You may obtain a copy of the License at
**
**      http://www.apache.org/licenses/LICENSE-2.0
**
**  Unless required by applicable law or agreed to in writing, software
**  distributed under the License is distributed on an "AS IS" BASIS,
**  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
**  See the License for the specific language governing permissions and
**  limitations under the License.
*/

#include <zephyr.h>
#include <stdio.h>
#include <logging/log.h>
#include <net/socket.h>

#include "gps.h"
#include "messagebuffer.h"
#include "version.h"
#include "init.h"
#include "chipcap2.h"
#include "n2_offload.h"
#include "fota.h"
#include "comms.h"
#include <net/coap.h>

#define LOG_LEVEL CONFIG_MAIN_LOG_LEVEL
LOG_MODULE_REGISTER(MAIN);

#define SEND_INTERVAL_SEC 30
#define FOTA_CHECK_INTERVAL_SEC 500

#define FOTA_COUNTER (FOTA_CHECK_INTERVAL_SEC / SEND_INTERVAL_SEC)

#define MAX_SEND_BUFFER 450
static uint8_t buffer[MAX_SEND_BUFFER];
static uint8_t coap_buffer[MAX_SEND_BUFFER];
#define SAMPLE_COAP_PATH "aq"
#define SAMPLE_COAP_SERVER "172.16.15.14"
#define SAMPLE_COAP_PORT 5683

int send_samples(uint8_t *data_buffer, size_t len)
{
	struct coap_packet p;
	int err = 0;
#define TOKEN_SIZE 8
	char token[TOKEN_SIZE];
	sys_csrand_get(token, TOKEN_SIZE);

	if (coap_packet_init(&p, coap_buffer, sizeof(coap_buffer), 1, COAP_TYPE_CON,
						 TOKEN_SIZE, token, COAP_METHOD_POST,
						 coap_next_id()) < 0)
	{
		LOG_ERR("Unable to iniitialize CoAP packet");
		return -1;
	}
	if (coap_packet_append_option(&p, COAP_OPTION_URI_PATH,
								  SAMPLE_COAP_PATH,
								  strlen(SAMPLE_COAP_PATH)) < 0)
	{
		LOG_ERR("Could not append path option to packet");
		return -1;
	}
	if (coap_packet_append_payload_marker(&p) < 0)
	{
		LOG_ERR("Unable to append payload marker to packet");
		return -1;
	}
	if (coap_packet_append_payload(&p, data_buffer, len) < 0)
	{
		LOG_ERR("Unable to append payload to CoAP packet");
		return -1;
	}

	modem_restart_without_triggering_network_signalling_storm_but_hopefully_picking_up_the_correct_cell___maybe();

	int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (sock < 0)
	{
		LOG_ERR("Error opening socket: %d", sock);
		return sock;
	}

	static struct sockaddr_in remote_addr = {
		sin_family : AF_INET,
	};
	remote_addr.sin_port = htons(SAMPLE_COAP_PORT);

	net_addr_pton(AF_INET, SAMPLE_COAP_SERVER, &remote_addr.sin_addr);

	int err = sendto(sock, coap_buffer, p.offset, 0, (struct sockaddr *)&remote_addr, sizeof(remote_addr));
	if (err < 0)
	{
		LOG_ERR("Unable to send data: %d", err);
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
	received = recvfrom(sock, coap_buffer, sizeof(coap_buffer), 0, (struct sockaddr *)&remove_addr, sizeof(remote_addr));
	if (received < 0)
	{
		LOG_ERR("Error receiving data: %d", received);
		close(sock);
		return received;
	}

	close(sock);

	struct coap_packet resp;

	return coap_packet_parse(&resp, coap_buffer, received, NULL, 0);
}

void main(void)
{
	init_board();

	// TODO: Watchdog init

	LOG_INF("This is the AQ node with version %s (%s)", AQ_VERSION, AQ_NAME);

	cc2_init();
	gps_init();
	opc_init();

	LOG_INF("Waiting for modem");
	wait_for_sockets();
	LOG_DBG("Ready to run");

	fota_init();

	if (fota_run())
	{
		k_sleep(1000);
	}

	int fota_interval = 0;
	while (true)
	{
		LOG_DBG("Sampling sensors");
		SENSOR_NODE_MESSAGE last_message;

		opc_n3_sample_data();
		opc_n3_get_sample(&last_message.opc_sample);

		adc_sample_data();
		adc_get_sample(&last_message.afe3_sample);

		cc2_sample_data();
		cc2_get_sample(&last_message.cc2_sample);

		gps_get_sample(&last_message.gps_fix);

		last_message.uptime = k_uptime_get() / 1000;

		int len = mb_encode(&last_message, buffer, MAX_SEND_BUFFER);
		LOG_DBG("Sample complete, encoded buffer = %d bytes", len);
#if 1
		mb_hex_dump_message(buffer, len);
#endif

		int ret = send_samples(buffer, len);
		if (ret < len)
		{
			LOG_WRN("Could not send message to server: %d", ret);
		}
		else
		{
			LOG_DBG("Successfully sent %d bytes to backend", len);
		}
		LOG_DBG("Done sending, sending again in %d seconds", SEND_INTERVAL_SEC);
		k_sleep(SEND_INTERVAL_SEC * K_MSEC(1000));

		fota_interval++;
		if (fota_interval > FOTA_COUNTER)
		{
			LOG_DBG("Check for new firmware");
			fota_interval = 0;
			if (fota_run())
			{
				k_sleep(1000);
			}
		}
#if 1
		mb_dump_message(&last_message);
#endif
	}
}
