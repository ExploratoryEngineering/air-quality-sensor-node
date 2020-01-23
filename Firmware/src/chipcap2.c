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
#include <logging/log.h>
#include <device.h>
#include <i2c.h>
#include "i2c_config.h"


// Note: The Chipcap sensor is onlys used for monitoring ambient temperature and humidity in the controller housing
//       The particle and gas sensors will provide temperature and RH data for the environmentl measurements. 

#define LOG_LEVEL CONFIG_EE06_LOG_LEVEL
LOG_MODULE_DECLARE(EE06);

float RH;
float Temp_C;

#define CC2_VALID_DATA (0x00)
#define CC2_STALE_DATA (0x01)

struct device * i2c_dev;

void CC2_init() 
{
    LOG_DBG("Initializing ChipCap2 ");

    i2c_dev = get_I2C_device();
    uint8_t NORMAL_OPERATION_MODE[] = {CHIPCAP2_NORMAL_OPERATION_MODE,0,0};
    int err = i2c_write(i2c_dev, NORMAL_OPERATION_MODE, 3, CHIPCAP2_ADDRESS);
    if (0 != err)
    {
        LOG_ERR("Unable to set Chipcap2 sensor in normal operations mode. i2c_write failed with error: %d", err);
    }
}

void CC2_sample()
{
    LOG_DBG("Sampling Chipcap2 sensor.");

    uint8_t rxBuffer[] = {0,0,0,0};
    int err = i2c_read(i2c_dev, rxBuffer, 4, CHIPCAP2_ADDRESS);
    if (0 != err)
    {
        LOG_ERR("Unable to get Chipcap2 sensor reading. i2c_read failed with error: %d", err);
    }

    // uint8_t status = rxBuffer[0] >> 6;

    float RH_H = (rxBuffer[0] & 0b00111111);
    float RH_L = rxBuffer[1];
    RH = ((RH_H*256 + RH_L)/16384)*100;
    float TempC_H = rxBuffer[2];
    float TempC_L = rxBuffer[3] >> 4;
    Temp_C = ((TempC_H*64+TempC_L)/16384)*165-40; 

    // Send a new measurement request after reading. Cannot be sent before first read
    uint8_t MEASUREMENT_REQUEST[] = {};

    err = i2c_write(i2c_dev, MEASUREMENT_REQUEST, 1, CHIPCAP2_ADDRESS);
    if (0 != err)
    {
        LOG_ERR("Unable to send measurement request to Chipcap2 sensor. i2c_write failed with error: %d", err);
    }

    // LOG_INF("Chipcap2: Temperature: %d", (int)Temp_C);
    LOG_INF("Chipcap2: Relative humidity: %d", (int)RH);
}

void CC2_entry_point(void * foo, void * bar, void * gazonk)
{
    LOG_INF("CC2 Thread running...");
   	CC2_init();
    while (true) 
    {
        k_sched_lock();
        CC2_sample();
        k_sched_unlock();
        k_sleep(30000);
    }
}

