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

#ifndef _MAX14830_h_
#define _MAX14830_h_

#include <stdint.h>

#define EE_NBIOT_01_ADDRESS 0x61

#define RX_BUFFER_SIZE 256

typedef void (*max_char_callback_t)(uint8_t data);

void init_max14830();

int max_send(const uint8_t *buffer, uint8_t len);

void max_init(max_char_callback_t cb);

#endif
