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

#define LOG_LEVEL CONFIG_EE06_LOG_LEVEL
LOG_MODULE_REGISTER(EE06);

#define GPS_THREAD_PRIORITY -5
#define GPS_THREAD_STACK_SIZE 1024
#define CC2_THREAD_PRIORITY -6
#define CC2_THREAD_STACK_SIZE 1024
#define OPC_THREAD_PRIORITY -4
#define OPC_THREAD_STACK_SIZE 1024

// K_THREAD_DEFINE(gps_thread_id, GPS_THREAD_STACK_SIZE, GPS_entry_point, NULL, NULL, NULL, GPS_THREAD_PRIORITY, 0, 30000);
// K_THREAD_DEFINE(cc2_thread_id, CC2_THREAD_STACK_SIZE, CC2_entry_point, NULL, NULL, NULL, GPS_THREAD_PRIORITY, 0, 20000);
K_THREAD_DEFINE(opc_thread_id, OPC_THREAD_STACK_SIZE, OPC_entry_point, NULL, NULL, NULL, OPC_THREAD_PRIORITY, 0, 25000);

void initialize_board()
{
	GPIO_init();
	I2C_init();
	SPI_init();
	ADS124S08_init();
	ADS124S08_begin();
}

void main(void)
{
	printk("TK - Alphasense\n");
	k_sleep(5000);	// (Testing only) Delay for manual powercycling and JLink

	initialize_board();

	while (1)
	{
		k_sleep(5000);
	}

}
