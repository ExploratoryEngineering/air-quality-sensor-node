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
#include "opc_n3.h"
#include "gpio.h"
#include "pinout.h"
#include <spi.h>
#include "spi_config.h"
#include <logging/log.h>

#define LOG_LEVEL CONFIG_EE06_LOG_LEVEL
LOG_MODULE_DECLARE(EE06);


struct spi_config OPC_config;
//struct spi_cs_control OPC_control;
struct spi_buf OPC_buf_tx;
struct spi_buf_set OPC_buf_set_tx;
struct spi_buf OPC_buf_rx;
struct spi_buf_set OPC_buf_set_rx;
struct device * OPC_spi_dev;

u8_t OPC_spi_tx_buffer[SPI_BUF_SIZE];
u8_t OPC_spi_rx_buffer[SPI_BUF_SIZE];

u8_t histogram[OPC_HISTOGRAM_SIZE];

#define INFORMATION_STRING_LENGTH 60

void OPC_selectDeviceCSLow(void)
{
	digitalWrite( CS_OPC, LOW );
}

void OPC_releaseChipSelect(void)
{
	digitalWrite( CS_OPC, HIGH );
}

void OPC_init()
{
    LOG_INF("OPC_N3 - Init.");

    OPC_spi_dev = SPI_init();
    OPC_config.cs = NULL;
    OPC_config.frequency = 500000;
    OPC_config.operation = SPI_OP_MODE_MASTER | SPI_WORD_SET(8) | SPI_TRANSFER_MSB | SPI_MODE_CPHA;
    OPC_config.slave = 0;

    LOG_INF("OPC_config cs: %d", (int)OPC_config.cs);
	LOG_INF("OPC_config frequency: %d", (int)OPC_config.frequency);
	LOG_INF("OPC_config operation: %d", (int)OPC_config.operation);
	LOG_INF("OPC_config slave: %d", (int)OPC_config.slave);
}


OPC_N3_RESULT OPC_command(uint8_t command_byte, uint8_t option_byte, uint8_t * buffer, int rxBytes)
{
    
    OPC_spi_tx_buffer[0] = command_byte;
	OPC_spi_rx_buffer[0] = 0x00;

	OPC_buf_tx.buf = &OPC_spi_tx_buffer;
	OPC_buf_tx.len = 1;
	OPC_buf_set_tx.buffers = &OPC_buf_tx;
	OPC_buf_set_tx.count = 1;

	OPC_buf_rx.buf = &OPC_spi_rx_buffer;
	OPC_buf_rx.len = 1;
	OPC_buf_set_rx.buffers = &OPC_buf_rx;
	OPC_buf_set_rx.count = 1;
    int err;
    int bufferIndex = 0;

    k_sleep(10); // ref manual [2.4]

    err = spi_transceive(OPC_spi_dev, &OPC_config, &OPC_buf_set_tx, &OPC_buf_set_rx);
	if (err)
	{
		LOG_ERR("OPC read information string failed (1) with error: %d ", err);
	}
    if (OPC_spi_rx_buffer[0] != 0x31)
    {
		LOG_ERR("Unexpected response byte 1 (%02X)from OPC-N3 when sending command : %02X", OPC_spi_rx_buffer[0], command_byte);
    }
    k_sleep(10);
    
    int retryCount = 0;
    while (true) 
    {
        int err = spi_transceive(OPC_spi_dev, &OPC_config, &OPC_buf_set_tx, &OPC_buf_set_rx);
	    if (err)
	    {
		    LOG_ERR("OPC_command failed (2) with error: %d, Retry count: %d", err, retryCount);
	    }
        if (OPC_spi_rx_buffer[0] == OPC_N3_DATA_READY) 
        {
            break;
        }
        if (retryCount++ > 20)
        {
            OPC_releaseChipSelect();
            LOG_ERR("Giving up. Too many retries...");
            return OPC_UNEXPECTED_OPC_RESPONSE;
        }
        k_sleep(OPC_N3_SPI_BUFFER_RESET_WAIT);
    }

    k_sleep(10);

    if (option_byte != OPC_OPTION_NONE)
    {
        OPC_spi_tx_buffer[0] = option_byte;
        OPC_spi_rx_buffer[0] = 0x00;
        err = spi_transceive(OPC_spi_dev, &OPC_config, &OPC_buf_set_tx, &OPC_buf_set_rx);
        if (err)
        {
            LOG_ERR("OPC read information string failed (1) with error: %d ", err);
        }
        if (OPC_spi_rx_buffer[0] != 0x03) {
            LOG_ERR("OPC unexpected response (%d) to option byte: %d ", OPC_spi_rx_buffer[0], option_byte);
        }
    }

    if (rxBytes != 0) 
    {
        memset(OPC_spi_tx_buffer, command_byte, SPI_BUF_SIZE);
        memset(OPC_spi_rx_buffer, 0x00, SPI_BUF_SIZE);

        for (int i=0; i<rxBytes; i++) 
        {
            err = spi_transceive(OPC_spi_dev, &OPC_config, &OPC_buf_set_tx, &OPC_buf_set_rx);
            if (err)
            {
                LOG_ERR("spi_transceive failed with error: %d", err);
            }
            if ((NULL != buffer) & (bufferIndex < SPI_BUF_SIZE))
            {
                buffer[bufferIndex] = OPC_spi_rx_buffer[0];
                bufferIndex++;
            }
        }
    }
    k_sleep(10);
    return OPC_OK;
}

void OPC_read_information_string()
{
    u8_t info[SPI_BUF_SIZE];
    memset(info, 0, SPI_BUF_SIZE);

    OPC_selectDeviceCSLow();
    OPC_command(OPC_N3_READ_INFORMATION_STRING, OPC_OPTION_NONE, &info[0], 60);
    OPC_releaseChipSelect();

    printk("[");
    for (int i=0; i< 60; i++)
    {
        printk("%c", info[i]);
    }
    printk("]\n");
    k_sleep(100);
}

void OPC_trace_historgram()
{
    printk("OPC-N3 histogram");
    for (int i=0; i<OPC_HISTOGRAM_SIZE; i++)  
    {
        printk("%d:%d ", i, histogram[i]);
        k_sleep(10);
    }
    k_sleep(100);
    printk("\n");
}

uint16_t get_uint16_value(uint8_t * buffer)
{
  union histogram_t 
  {
    uint8_t b[2];
    uint16_t value;
  } h;

  h.b[0] = *(uint8_t *)buffer;
  h.b[1] = *(uint8_t *)(buffer+1);

  return h.value;
}

float get_float_value(uint8_t * buffer)
{
  union histogram_t 
  {
    uint8_t b[4];
    float value;
  } h;

  h.b[0] = *(uint8_t *)buffer;
  h.b[1] = *(uint8_t *)(buffer+1);
  h.b[2] = *(uint8_t *)(buffer+2);
  h.b[3] = *(uint8_t *)(buffer+3);

  return h.value;
}

uint16_t OPC_calcCRC(uint8_t data[], uint8_t number_of_bytes)
{
    #define PLYNOMIAL   0xA001
    #define InitCRCval  0xFFFF

    uint8_t     bit;
    uint16_t    crc = InitCRCval;
    uint8_t     byteCounter;

    for (byteCounter=0; byteCounter < number_of_bytes; byteCounter++)
    {
        crc ^= (uint16_t)data[byteCounter];
        for (bit=0; bit<8; bit++) 
        {
            if (crc & 0b00000001)
            {
                crc >>= 1;
                crc ^= PLYNOMIAL;
            }
            else 
            {
                crc >>= 1;
            }
        }
    }
    return crc;
}

OPC_N3_RESULT OPC_sample()
{
    OPC_selectDeviceCSLow();

    LOG_INF("Switching fan - ON");
    OPC_command(OPC_N3_WRITE_PERIPHERAL_POWER_STATUS, OPC_OPTION_FAN_ON, NULL, 0);
    k_sleep(FAN_SETTLING_TIME_MS);

    LOG_INF("Switching laser - ON");
    OPC_command(OPC_N3_WRITE_PERIPHERAL_POWER_STATUS, OPC_OPTION_LASER_ON, NULL, 0);
    k_sleep(LASER_SETTLING_TIME_MS);

    LOG_INF("Switching *MYSTERYSWITCH* - ON");
    OPC_command(OPC_N3_WRITE_PERIPHERAL_POWER_STATUS, 0x05, NULL, 0);
    k_sleep(LASER_SETTLING_TIME_MS);

    LOG_INF("Sampling");
    k_sleep(OPC_SAMPLING_TIME_MS);

    LOG_INF("Initiating transmission of histogram...");
    memset(histogram, 0, OPC_HISTOGRAM_SIZE);

    OPC_command(OPC_N3_READ_HISTOGRAM_DATA_AND_RESET_HISTOGRAM, OPC_OPTION_NONE, &histogram[0], OPC_HISTOGRAM_SIZE);

    OPC_trace_historgram();

    LOG_INF("Switching laser - OFF");
    OPC_command(OPC_N3_WRITE_PERIPHERAL_POWER_STATUS, OPC_OPTION_LASER_OFF, NULL, 0);
    k_sleep(LASER_SETTLING_TIME_MS);

    LOG_INF("Switching fan - OFF");
    OPC_command(OPC_N3_WRITE_PERIPHERAL_POWER_STATUS, OPC_OPTION_FAN_OFF, NULL, 0);
    k_sleep(FAN_SETTLING_TIME_MS);

    OPC_releaseChipSelect();

    // Sanity check
    uint16_t checksum = get_uint16_value(&histogram[OPC_HISTOGRAM_CHECKSUM_INDEX]);
    if (checksum == OPC_calcCRC(histogram, OPC_HISTOGRAM_SIZE-2))
    {
        LOG_INF("Yay! We have a valid checksum! (%04X)", checksum);
    }
    else
    {
        LOG_ERR("CRC Checksum error in histogram! (%04X)", checksum);
        LOG_INF("CRC byte 1 : %02X", histogram[OPC_HISTOGRAM_CHECKSUM_INDEX]);
        LOG_INF("CRC byte 2 : %02X", histogram[OPC_HISTOGRAM_CHECKSUM_INDEX+1]);
        return OPC_CRC_ERROR;
    }

    return OPC_OK;
}


void OPC_entry_point(void * foo, void * bar, void * gazonk)
{
   	OPC_init();
    OPC_read_information_string();

    //float pm_a,pm_b,pm_c;

    while (true) 
    {
        LOG_INF("OPC-N3 Thread running...");
        k_sleep(1);
        k_sched_lock();
        OPC_sample();
        k_sched_unlock();


        //OPC_trace_historgram();
        k_sleep(30000);
    }
}

