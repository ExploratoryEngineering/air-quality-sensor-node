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
#include "chipcap2.h"
#include <stdio.h>
#include <device.h>
#include <i2c.h>
#include "i2c_config.h"
#include "messagebuffer.h"

// Note: The Chipcap sensor is onlys used for monitoring ambient temperature and humidity in the controller housing
//       The particle and gas sensors will provide temperature and RH data for the environmentl measurements.

#define LOG_LEVEL CONFIG_CHIPCAP2_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(CHIPCAP2);

#define CC2_VALID_DATA (0x00)
#define CC2_STALE_DATA (0x01)

int cc2_init()
{
    uint8_t NORMAL_OPERATION_MODE[] = {CHIPCAP2_NORMAL_OPERATION_MODE, 0, 0};
    return i2c_write(get_I2C_device(), NORMAL_OPERATION_MODE, 3, CHIPCAP2_ADDRESS);
}

static CC2_SAMPLE current;

void cc2_sample_data()
{
    int err = cc2_init();
    if (err)
    {
        LOG_ERR("Unable to set Chipcap2 sensor in normal operations mode. i2c_write failed with error: %d", err);
        return;
    }

    struct device *i2c_device = get_I2C_device();
    uint8_t rxBuffer[] = {0, 0, 0, 0};
    err = i2c_read(i2c_device, rxBuffer, 4, CHIPCAP2_ADDRESS);
    if (0 != err)
    {
        LOG_ERR("Unable to get Chipcap2 sensor reading. i2c_read failed with error: %d", err);
        return;
    }

    // uint8_t status = rxBuffer[0] >> 6;
    float RH_H = (rxBuffer[0] & 0b00111111);
    float RH_L = rxBuffer[1];
    current.RH = ((RH_H * 256 + RH_L) / 16384) * 100;
    float TempC_H = rxBuffer[2];
    float TempC_L = rxBuffer[3] >> 4;
    current.Temp_C = ((TempC_H * 64 + TempC_L) / 16384) * 165 - 40;

    // Send a new measurement request after reading. Cannot be sent before first read
    //    uint8_t measurement_request = 1;

    err = i2c_write(i2c_device, NULL, 0, CHIPCAP2_ADDRESS);
    if (0 != err)
    {
        LOG_ERR("Unable to send measurement request to Chipcap2 sensor. i2c_write failed with error: %d", err);
    }
}

void cc2_get_sample(CC2_SAMPLE *msg)
{
    memcpy(msg, &current, sizeof(current));
}