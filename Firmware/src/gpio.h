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

#ifndef _GPIO_H_
#define _GPIO_H_

#include <stdint.h>
#include <zephyr.h>
#include <device.h>

#define GPIO_DRV_NAME "GPIO_NRF_P0"

// For ADS124s08 code
#define LOW 0
#define HIGH 1
int digitalWrite( u32_t pin, u32_t value );


void init_GPIO();
struct device * get_GPIO_device();
int ConfigureOutputPin(u32_t pin);
int ConfigureInputPin(u32_t pin);
struct device * get_GPIO_device();
int UnSelect(u32_t pin);
void UnselectAllSPI();
int Select(u32_t pin);
void gps_reset();

#endif // _GPIO_H_