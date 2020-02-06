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
#include <stdio.h>
#include <uart.h>
#include "gps.h"
#include "gps_cache.h"
#include "gpio.h"
#include "messagebuffer.h"

#define LOG_LEVEL CONFIG_GPS_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(GPS);

#define GPS_FIX_RETRY_LIMIT 120
#define MAX_NMEA_BUFFER 254

static uint8_t nmea_buffer[MAX_NMEA_BUFFER];
static uint8_t nmea_pos = 0;

int gps_retries_before_fix = 0;

static int rxData(uint8_t data)
{
    nmea_buffer[nmea_pos++] = data;
    if (nmea_pos >= MAX_NMEA_BUFFER)
    {
        nmea_pos = 0;
        return -1;
    }

    if (data == '\n')
    {
        nmea_buffer[nmea_pos] = 0;
        nmea_sentence_t sentence;
        if (nmea_parse(nmea_buffer, &sentence))
        {
            if (sentence.type[0] == 'G' && sentence.type[2] == 'A')
            {
                if (sentence.type[1] == 'G')
                {
                    // a very convoluted way of saying 'GGA'
                    gps_gga_t gga;
                    nmea_decode_gga(&sentence, &gga);
                    gps_update_gga(&gga);
                }
                else if (sentence.type[1] == 'S')
                {
                    // a very convoluted way of saying 'GSA'
                    gps_gsa_t gsa;
                    nmea_decode_gsa(&sentence, &gsa);
                    gps_update_gsa(&gsa);
                }
            }
            else if (sentence.type[0] == 'R' && sentence.type[1] == 'M' && sentence.type[2] == 'C')
            {
                gps_rmc_t rmc;
                nmea_decode_rmc(&sentence, &rmc);
                gps_update_rmc(&rmc);
            }
            else if (sentence.type[0] == 'G' && sentence.type[1] == 'S' && sentence.type[2] == 'V')
            {
                /* GSV - detailed satellite data
                $GPGSV,2,1,08,01,40,083,46,02,17,308,41,12,07,344,39,14,22,228,45*75

                Where:
                    GSV          Satellites in view
                    2            Number of sentences for full data
                    1            sentence 1 of 2
                    08           Number of satellites in view

                    01           Satellite PRN number
                    40           Elevation, degrees
                    083          Azimuth, degrees
                    46           SNR - higher is better
                        for up to 4 satellites per sentence
                    *75          the checksum data, always begins with *
                */
                LOG_DBG("GSV: talker: %c%c  satellites: %s", sentence.talker[0], sentence.talker[1], log_strdup((char *)sentence.fields[3]));
            }
            else if (sentence.type[0] == 'V' && sentence.type[1] == 'T' && sentence.type[2] == 'G')
            {
                gps_vtg_t vtg;
                nmea_decode_vtg(&sentence, &vtg);
                gps_update_vtg(&vtg);
            }
            else
            {
                LOG_DBG("Unknown sentence: %s", log_strdup(nmea_buffer));
            }
        }
        nmea_pos = 0;
        memset(nmea_buffer, 0, MAX_NMEA_BUFFER);
    }
    return 0;
}

static void uart_fifo_callback(struct device *dev)
{
    u8_t recvData;
    int err = uart_irq_update(dev);
    if (err != 1)
    {
        LOG_ERR("uart_fifo_callback. uart_irq_update failed with error: %d (expected 1)", err);
        return;
    }

    if (uart_irq_rx_ready(dev))
    {
        uart_fifo_read(dev, &recvData, 1);
        rxData(recvData);
    }
}

#define GPS_LOOP_SLEEP 5000

static gps_fix_t current;

static void GPS_entry_point(void *foo, void *bar, void *gazonk)
{
    struct device *uart_dev = device_get_binding("UART_0");
    if (!uart_dev)
    {
        LOG_ERR("Unable to load UART device. GPS Thread cannot continue.");
        return;
    }
    uart_irq_callback_set(uart_dev, uart_fifo_callback);
    uart_irq_rx_enable(uart_dev);
    LOG_DBG("UART device loaded.");

    while (true)
    {
        k_sched_lock();
        if (gps_get_fix(&current))
        {
            LOG_DBG("----- GPS has fix : %d, %d\n", (int)(current.longitude * 1000), (int)(current.latitude * 1000));
        }
        else
        {
            gps_retries_before_fix++;
            if (gps_retries_before_fix > GPS_FIX_RETRY_LIMIT)
            {
                gps_retries_before_fix = 0;
                gps_reset();
            }
        }
        k_sched_unlock();

        k_sleep(GPS_LOOP_SLEEP);
    }
}

#define GPS_THREAD_PRIORITY -5
#define GPS_THREAD_STACK_SIZE 1024

struct k_thread gps_thread;

K_THREAD_STACK_DEFINE(gps_thread_stack, GPS_THREAD_STACK_SIZE);

void gps_init()
{
    LOG_INF("Init");
    k_thread_create(&gps_thread, gps_thread_stack,
                    K_THREAD_STACK_SIZEOF(gps_thread_stack),
                    (k_thread_entry_t)GPS_entry_point,
                    NULL, NULL, NULL, GPS_THREAD_PRIORITY, 0, K_NO_WAIT);
}

void gps_get_sample(gps_fix_t *msg)
{
    memcpy(msg, &current, sizeof(current));
}