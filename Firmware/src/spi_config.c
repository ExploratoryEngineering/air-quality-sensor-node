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
#include <spi.h>
#include "spi_config.h"

#define LOG_LEVEL CONFIG_EE06_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(SPI_CONFIG);

struct device * spi_dev;

struct device * get_SPI_device()
{
  spi_dev = device_get_binding(SPI_DEV);
  if (!spi_dev) {
    LOG_ERR("SPI device driver not found");
    return NULL;
  }

  return spi_dev;
}