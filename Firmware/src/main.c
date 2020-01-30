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

#define LOG_LEVEL CONFIG_EE06_LOG_LEVEL
LOG_MODULE_REGISTER(EE06);

#define GPS_THREAD_PRIORITY -5
#define GPS_THREAD_STACK_SIZE 1024
// #define MAX_THREAD_PRIORITY -4
// #define MAX_THREAD_STACK_SIZE 1024

K_THREAD_DEFINE(gps_thread_id, GPS_THREAD_STACK_SIZE, GPS_entry_point, NULL, NULL, NULL, GPS_THREAD_PRIORITY, 0, 30000);
// K_THREAD_DEFINE(max_thread_id, MAX_THREAD_STACK_SIZE, MAX_entry_point, NULL, NULL, NULL, MAX_THREAD_PRIORITY, 0, 15000);

struct device * gpio_device;

bool initialize_board()
{
	gpio_device = get_GPIO_device();
	init_GPIO(gpio_device);
	if (NULL == gpio_device) {
		LOG_ERR("Unable to initialize GPIO device");
		return false;
	}
	if (NULL == get_I2C_device()) {
		LOG_ERR("Unable to initialize I2C device");
		return false;
	}
	if (NULL == get_SPI_device()) {
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
	printk("EE-04\n");
	while (1)
	{
		k_sleep(2000);
		LOG_INF("Nothing to see here...");
	}
#else
	printk("Air quality sensor node\n");
	k_sleep(5000);	// (Testing only) Delay for manual powercycling and JLink

	if (!initialize_board())
	{
		LOG_ERR("Board initialization failed. Unable to start sensor node.");
		return;
	}

	LOG_INF("Message size: %d", sizeof(SENSOR_NODE_MESSAGE));


	while (1)
	{
		CC2_main(NULL, NULL, NULL);
		ADC_main(NULL, NULL, NULL);
		OPC_main(NULL, NULL, NULL);

		DEBUG_CC2();
		DEBUG_GPS();
		DEBUG_AFE();
		DEBUG_OPC();

		k_sleep(5000);
	}
#endif
}
