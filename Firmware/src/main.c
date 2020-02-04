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
#include <device.h>
#include <stdio.h>
#include <logging/log.h>

#include "pinout.h"
#include "gps.h"
#include "messagebuffer.h"
#include "version.h"

#define LOG_LEVEL CONFIG_EE06_LOG_LEVEL
LOG_MODULE_REGISTER(EE06);


extern SENSOR_NODE_MESSAGE sensor_node_message;

void main(void)
{
	LOG_INF("This is the AQ node with version %s (%s).\n", AQ_VERSION, AQ_NAME);

	gps_init();

	LOG_INF("Message size: %d", sizeof(SENSOR_NODE_MESSAGE));

	while (1)
	{
		// LOG_INF("Sampling...");
		// CC2_main(NULL, NULL, NULL);
		// ADC_main(NULL, NULL, NULL);
		// OPC_main(NULL, NULL, NULL);

		// TODO:
		// 	1) encodoe + TX
		// 	2) TX interval
		// 	3) FOTA
		//  4) Updtime info (AFE warmup meta)
		//	5) Register AFE serial in Horde
		//	6) Version number in message

		// DEBUG_CC2();
		// DEBUG_GPS();
		// DEBUG_AFE();
		// DEBUG_OPC();

		k_sleep(5000);
	}
}
