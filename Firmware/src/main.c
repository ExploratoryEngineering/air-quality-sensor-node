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

#include "gps.h"
#include "messagebuffer.h"
#include "version.h"

#define LOG_LEVEL CONFIG_EE06_LOG_LEVEL
LOG_MODULE_REGISTER(EE06);

#define MIN_SEND_INTERVAL K_MSEC(60 * 1000)

void main(void)
{
	// TODO: Watchdog init

	// TODO: FOTA init and processing

	// fota_init();

	LOG_INF("This is the AQ node with version %s (%s).\n", AQ_VERSION, AQ_NAME);

	gps_init();

	while (true)
	{
		k_sleep(MIN_SEND_INTERVAL);
/*
		SENSOR_NODE_MESSAGE last_message;
		n3_wait_for_sample();

		if (!n3_get_sample(&last_message)) {
			LOG_WRN("Could not read OPC N3 sample");
			continue;
		}

		if (!cc2_get_sample(&last_message)) {
			LOG_WRN("Could not read ChipCap2 sample");
			continue;
		}
		if (!adc_get_sample(&last_message)) {
			LOG_WRN("Could not read ADC sample");
			continue;
		}
		if (!gps_get_sample(&last_message)) {
			LOG_WRN("Could not read GPS sample");
			continue;
		}

		if (!send_nbiot_message(&last_message)) {
			LOG_WRN("Could not send message to server");
		}*/
	}
}
