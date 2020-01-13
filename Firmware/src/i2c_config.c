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
#include "i2c_config.h"
#include <misc/printk.h>
#include <device.h>
#include <i2c.h>
#include <logging/log.h>


LOG_MODULE_DECLARE(EE06);

#define I2C_DEV "I2C_0"

struct device * I2C_init() 
{
	struct device * i2c_dev;
	LOG_INF("I2C_init");
	i2c_dev = device_get_binding(I2C_DEV);
	if (!i2c_dev) {
		LOG_ERR("I2C device driver not found");
		return NULL;
	}
    if (i2c_configure(i2c_dev, I2C_SPEED_SET(I2C_SPEED_FAST))) {
		LOG_ERR("I2C configuration failed");
        return NULL;
    }

    return i2c_dev;
}

void I2CScan() 
{
	struct device * i2c_dev = I2C_init();

	for (u8_t i = 4; i <= 0x77; i++) 
	{
		struct i2c_msg msgs[1];
		u8_t dst;

		printk("Checking address : %02X\n", i);
		/* Send the address to read from */
		msgs[0].buf = &dst;
		msgs[0].len = 0U;
		msgs[0].flags = I2C_MSG_WRITE | I2C_MSG_STOP;

		if (i2c_transfer(i2c_dev, &msgs[0], 1, i) == 0) {
			printk("0x%2x FOUND\n", i);
		}
		k_sleep(100);
	}
}