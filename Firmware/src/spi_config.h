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

#ifndef _SPI_CONFIG_H_
#define _SPI_CONFIG_H_

#include <zephyr.h>

#define SPI_BUF_SIZE 256

struct device * get_SPI_device();

#define SPI_DEV "SPI_1"

extern struct device * spi_dev;
extern struct device * gpio_dev;

#endif // _SPI_CONFIG_H_