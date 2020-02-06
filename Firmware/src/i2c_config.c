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
#include <stdio.h>
#include <device.h>
#include <i2c.h>

#define LOG_LEVEL CONFIG_EEI2C_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(EEI2C);

static struct device *i2c_dev = NULL;

struct device *get_I2C_device()
{
	if (!i2c_dev)
	{
		i2c_dev = device_get_binding(I2C_DEV);
		if (!i2c_dev)
		{
			LOG_ERR("Device driver not found");
			return NULL;
		}
		if (i2c_configure(i2c_dev, I2C_SPEED_SET(I2C_SPEED_STANDARD)))
		{
			LOG_ERR("Configuration failed");
			return NULL;
		}
	}
	return i2c_dev;
}
