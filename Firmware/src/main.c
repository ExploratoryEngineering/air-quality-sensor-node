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
#include <misc/reboot.h>
#include <drivers/watchdog.h>

#include "gps.h"
#include "messagebuffer.h"
#include "version.h"
#include "init.h"
#include "chipcap2.h"
#include "n2_offload.h"
#include "fota.h"
#include "comms.h"
#include "config.h"

#define LOG_LEVEL CONFIG_MAIN_LOG_LEVEL
LOG_MODULE_REGISTER(MAIN);

#define SEND_INTERVAL_SEC 30
#define FOTA_CHECK_INTERVAL_SEC 500

#define FOTA_COUNTER (FOTA_CHECK_INTERVAL_SEC / SEND_INTERVAL_SEC)

#define MAX_SEND_BUFFER 450
static uint8_t buffer[MAX_SEND_BUFFER];
#define MAX_COMMAND_BUFFER 1024
static uint8_t command_buffer[MAX_COMMAND_BUFFER];

// Watchdog
#define WDT_DEV_NAME DT_WDT_0_NAME
struct device *wdt;
int wdt_channel_id;

extern char APN_NAME[NVS_APN_COUNT][APN_NAME_SIZE];  // Testing only. Remove

static void wdt_callback(struct device *wdt_dev, int channel_id)
{
	LOG_WRN("Watchdog timer expired. Rebooting in 1 second");
	k_sleep(1000);
	sys_reboot(0);
}

void watchdog_init()
{
	int err;
	struct wdt_timeout_cfg wdt_config;

	LOG_INF("Initializing watchdog");

	wdt = device_get_binding(WDT_DEV_NAME);
	if (!wdt) {
		printk("Cannot get WDT device\n");
		return;
	}

	// Reset SoC when watchdog timer expires.
	wdt_config.flags = WDT_FLAG_RESET_SOC;

	// Expire watchdog after two hours. 
	wdt_config.window.min = 0U;
	wdt_config.window.max = 7200000U;

	// Set up watchdog callback. Jump into it when watchdog expired.
	wdt_config.callback = wdt_callback;

	wdt_channel_id = wdt_install_timeout(wdt, &wdt_config);
	if (wdt_channel_id == -ENOTSUP) 
	{
		wdt_config.callback = NULL;
		wdt_channel_id = wdt_install_timeout(wdt, &wdt_config);
	}
	if (wdt_channel_id < 0) 
	{
		LOG_ERR("Watchdog install error");
		return;
	}

	err = wdt_setup(wdt, 0);
	if (err < 0) 
	{
		LOG_ERR("Watchdog setup error\n");
		return;
	}

	LOG_INF("Watchdog channel id: %d", wdt_channel_id);
}

void main(void)
{
 	init_board();
	watchdog_init();

	k_sleep(1000);

	LOG_INF("This is the AQ node with version %s (%s)", AQ_VERSION, AQ_NAME);

	cc2_init();
	gps_init();
	opc_init();

	LOG_INF("Waiting for modem");
	wait_for_sockets();
	LOG_DBG("Ready to run");

	select_active_apn();
	fota_init();
	if (fota_run())
	{
		k_sleep(1000);
	}

	// Main loop
	int fota_interval = 0;
	while (true)
	{
		modem_restart_without_triggering_network_signalling_storm_but_hopefully_picking_up_the_correct_cell___maybe();

		wdt_feed(wdt, wdt_channel_id); // Tickle and feed the watchdog


// Open a socket
	int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (sock < 0)
	{
		LOG_ERR("Error opening socket: %d", sock);
		k_sleep(1000);
		sys_reboot(0);
	}
	static struct sockaddr_in remote_addr = {
		sin_family : AF_INET,
	};
	remote_addr.sin_port = htons(1234);
	net_addr_pton(AF_INET, "172.16.15.14", &remote_addr.sin_addr);



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
	#if 0
		mb_hex_dump_message(buffer, len);
	#endif


		int err = sendto(sock, buffer, len, 0, (struct sockaddr *)&remote_addr, sizeof(remote_addr));
		if (err < 0)
		{
			LOG_ERR("Unable to send data: %d", err);
			k_sleep(5000);
			sys_reboot(0);
		}
		int received = recvfrom(sock, command_buffer, sizeof(command_buffer), 0, NULL, NULL);
		if (received == 0)
		{
			LOG_INF("No data received...");
		}
		else
		{
			LOG_INF("CONFIG message received");
#pragma message("TODO: Implement message decode / protobuf parsing")
			// TODO :
			//		- decode message
			//		- validate arguments
			//		- send response
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

		close (sock);

		// Force a reboot once a day. This wil implicitly trigger a new APN scan
		int uptime_seconds = k_uptime_get() / 1000;
		LOG_INF("Uptime: %d seconds", uptime_seconds);
		if (uptime_seconds > UPTIME_FORCE_REBOOT_LIMIT_SECONDS)
		{
			sys_reboot(0);
		}
		
#if 0
			mb_dump_message(&last_message);
#endif
	}


}
