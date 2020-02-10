#pragma once

#include <stdio.h>
#include <stdint.h>
#include <string.h>

#define FIRMWARE_VER_ID 1
#define MODEL_NUMBER_ID 2
#define SERIAL_NUMBER_ID 3
#define CLIENT_MANUFACTURER_ID 4

size_t encode_tlv_string(uint8_t *buf, uint8_t id, const uint8_t *str);
