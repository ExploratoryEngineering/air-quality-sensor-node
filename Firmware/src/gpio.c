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
#include "gpio.h"
#include "pinout.h"
#include <zephyr.h>
#include <device.h>
#include <gpio.h>
#include <misc/util.h>
#include <stdio.h>

#define LOG_LEVEL CONFIG_EE06_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(GPIO);

static struct device *gpio_dev = NULL;

int ConfigureOutputPin(u32_t pin)
{
    int ret = gpio_pin_configure(gpio_dev, pin, (GPIO_DIR_OUT));
    if (ret)
    {
        printf("Error configuring %d!\n", pin);
    }
    return ret;
}

int ConfigureInputPin(u32_t pin)
{
    int ret = gpio_pin_configure(gpio_dev, pin, (GPIO_DIR_IN));
    if (ret)
    {
        printf("Error configuring %d!\n", pin);
    }
    return ret;
}

static void init_GPIO()
{
    // CS_SX1276
    LOG_INF("Disable ANT_HF_CONTROL");
    ConfigureOutputPin(CS_SX1276);
    gpio_pin_write(gpio_dev, CS_SX1276, 1);

    // ADC
    LOG_INF("Configuring ADC IO");
    ConfigureOutputPin(CS_ADC);
    ConfigureOutputPin(ADC_SYNC);
    ConfigureOutputPin(ADC_RESET);
    // ConfigureOutputPin(CKEN_PIN);
    // ConfigureOutputPin(DRDY_PIN);
    gpio_pin_write(gpio_dev, ADC_SYNC, 0);
    gpio_pin_write(gpio_dev, ADC_RESET, 0);
    k_sleep(1000);
    gpio_pin_write(gpio_dev, ADC_RESET, 1);
    gpio_pin_write(gpio_dev, CS_ADC, 1);
    // gpio_pin_write(CK_EN, 0);

    // OPC-N3
    LOG_INF("Configuring OPC-N3 IO");
    ConfigureOutputPin(CS_OPC);
    gpio_pin_write(gpio_dev, CS_OPC, 1);

    // GPS
    LOG_INF("Configuring GPS IO");
    ConfigureOutputPin(GPS_FORCE_ON);
    ConfigureOutputPin(GPS_TX);
    ConfigureOutputPin(GPS_RESET);
    ConfigureOutputPin(GPS_WAKE_UP);
    ConfigureInputPin(GPS_RX);

    gpio_pin_write(gpio_dev, GPS_RESET, 0);
    gpio_pin_write(gpio_dev, GPS_RESET, 1);
    gpio_pin_write(gpio_dev, GPS_FORCE_ON, 0);
    k_sleep(1000);
    gpio_pin_write(gpio_dev, GPS_FORCE_ON, 1);

    // MAX14830
    LOG_INF("Configuring MAX14830 IO");
    ConfigureOutputPin(MAX14830_RESET);
    gpio_pin_write(gpio_dev, MAX14830_RESET, 1);
    gpio_pin_write(gpio_dev, MAX14830_RESET, 0);
    k_sleep(1000);
    gpio_pin_write(gpio_dev, MAX14830_RESET, 1);
    k_sleep(1000);
    int ret = gpio_pin_configure(gpio_dev, MAX14830_IRQ, GPIO_DIR_IN | GPIO_PUD_PULL_UP);
    if (ret)
    {
        printf("Error configuring %d: %d\n", MAX14830_IRQ, ret);
    }

    // EE-NBIOT-01/02
    ConfigureOutputPin(EE_NBIOT_01_RESET);
    gpio_pin_write(gpio_dev, EE_NBIOT_01_RESET, 1);

    LOG_INF("InitGPIO - done.");
}

struct device *get_GPIO_device()
{
    if (gpio_dev == NULL)
    {
        gpio_dev = device_get_binding(DT_GPIO_P0_DEV_NAME);
        if (!gpio_dev)
        {
            printf("YIKES ! Cannot find %s!\n", DT_GPIO_P0_DEV_NAME);
            return NULL;
        }
        init_GPIO();
    }
    return gpio_dev;
}


void gps_reset()
{
    LOG_INF("GPS reset...\n");

    gpio_pin_write(gpio_dev, GPS_RESET, 0);
    gpio_pin_write(gpio_dev, GPS_RESET, 1);
    gpio_pin_write(gpio_dev, GPS_FORCE_ON, 0);
    k_sleep(1000);
    gpio_pin_write(gpio_dev, GPS_FORCE_ON, 1);
}

int UnSelect(u32_t pin)
{
    int ret = gpio_pin_write(gpio_dev, pin, 1);
    if (ret)
    {
        printf("Error unselecting : %d!\n", pin);
    }
    return ret;
}

void UnselectAllSPI()
{
    UnSelect(CS_OPC);
    UnSelect(CS_ADC);
}

int Select(u32_t pin)
{
    UnselectAllSPI();

    int ret = gpio_pin_write(gpio_dev, pin, 0);
    if (ret)
    {
        printf("Error unselecting : %d!\n", pin);
    }
    return ret;
}

int digitalWrite(u32_t pin, u32_t value)
{
    int ret = gpio_pin_write(gpio_dev, pin, value);
    if (ret)
    {
        printf("Error writing %d to pin %d.\n", value, pin);
    }
    return ret;
}
