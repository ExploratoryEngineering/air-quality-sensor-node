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
#include <device.h>
#include <i2c.h>
#include <uart.h>

#include "pinout.h"
#include "spi.h"
#include "gpio.h"
#include "spi_config.h"
#include "opc_n3.h"
#include "i2c_config.h"
#include "spi_config.h"
#include "ads124s08.h"
#include "chipcap2.h"
#include "gps.h"
#include "max14830.h"
#include "messagebuffer.h"
#include "version.h"

#define LOG_LEVEL CONFIG_EE06_LOG_LEVEL
LOG_MODULE_REGISTER(EE06);

#define GPS_THREAD_PRIORITY -5
#define GPS_THREAD_STACK_SIZE 1024

K_THREAD_DEFINE(gps_thread_id, GPS_THREAD_STACK_SIZE, GPS_entry_point, NULL, NULL, NULL, GPS_THREAD_PRIORITY, 0, 30000);

struct device *gpio_device;
extern SENSOR_NODE_MESSAGE sensor_node_message;

bool initialize_board()
{
	gpio_device = get_GPIO_device();
	init_GPIO(gpio_device);
	if (NULL == gpio_device)
	{
		LOG_ERR("Unable to initialize GPIO device");
		return false;
	}
	if (NULL == get_I2C_device())
	{
		LOG_ERR("Unable to initialize I2C device");
		return false;
	}
	if (NULL == get_SPI_device())
	{
		LOG_ERR("Unable to initialize SPI device");
		return false;
	}
	ADS124S08_init();
	ADS124S08_begin();
	return true;
}

void main(void)
{
#ifdef EE_04
	printf("EE-04\n");
	while (1)
	{
		k_sleep(2000);
		LOG_INF("Nothing to see here...");
	}
#else
	LOG_INF("This is the AQ node with version %s (%s).\n", AQ_VERSION, AQ_NAME);

	k_sleep(5000); // (Testing only) Delay for manual powercycling and JLink

	if (!initialize_board())
	{
		LOG_ERR("Board initialization failed. Unable to start sensor node.");
		return;
	}

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
#endif
}
