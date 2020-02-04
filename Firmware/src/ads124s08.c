/* --COPYRIGHT--,BSD
 * Copyright (c) 2016, Texas Instruments Incorporated
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * --/COPYRIGHT--*/

#include <zephyr.h>
#include <spi.h>
#include <device.h>
#include <stdio.h>
#include "spi_config.h"
#include "ADS124S08.h"
#include <gpio.h>
#include "gpio.h"
#include "opc_n3.h"
#include "messagebuffer.h"

#define LOG_LEVEL CONFIG_EE06_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(ADS123S08);

uint8_t ADS124S08__drdy_pin;
uint8_t ADS124S08__start_pin;
uint8_t ADS124S08__reset_pin;
struct spi_config ADS124S08_config;
struct spi_cs_control ADS124S08_control;
struct spi_buf ADS124S08_buf_tx;
struct spi_buf_set ADS124S08_buf_set_tx;
struct spi_buf ADS124S08_buf_rx;
struct spi_buf_set ADS124S08_buf_set_rx;
struct device *ADS124S08_spi_dev;

extern SENSOR_NODE_MESSAGE sensor_node_message;

u8_t ADS124S08_spi_tx_buffer[SPI_BUF_SIZE];
u8_t ADS124S08_spi_rx_buffer[SPI_BUF_SIZE];

void ADS124S08_select(void)
{
	digitalWrite(CS_PIN, LOW);
	k_sleep(1);
}

void ADS124S08_release(void)
{
	digitalWrite(CS_PIN, HIGH);
	k_sleep(1);
}

void ADS124S08_init(void)
{
	LOG_INF("ADS124S08_init");
	ADS124S08_release();
	ADS124S08_stopConversion();
}

void ADS124S08_begin()
{
	LOG_DBG("ADS124S08_begin");

	ADS124S08_config.cs = NULL;
	ADS124S08_config.frequency = 1000000;
	ADS124S08_config.operation = SPI_OP_MODE_MASTER | SPI_WORD_SET(8) | SPI_TRANSFER_MSB | SPI_MODE_CPHA;
	ADS124S08_config.slave = 0;
	ADS124S08_spi_dev = device_get_binding(SPI_DEV);
	if (!ADS124S08_spi_dev)
	{
		LOG_ERR("SPI device driver not found");
	}

	LOG_INF("ADS124S08_config cs: %d", (int)ADS124S08_config.cs);
	LOG_INF("ADS124S08_config frequency: %d", (int)ADS124S08_config.frequency);
	LOG_INF("ADS124S08_config operation: %d", (int)ADS124S08_config.operation);
	LOG_INF("ADS124S08_config slave: %d", (int)ADS124S08_config.slave);
}

// Reads a single register contents from the specified address
//
// \param regnum identifies which address to read
//
char ADS124S08_regRead(unsigned int regnum)
{
	LOG_INF("ADS124S08_regRead - register: %d", regnum);
	ADS124S08_spi_tx_buffer[0] = REGRD_OPCODE_MASK + (regnum & 0x1f);
	ADS124S08_spi_tx_buffer[1] = 0x00;
	ADS124S08_spi_tx_buffer[2] = 0x00;

	ADS124S08_spi_rx_buffer[0] = 0x00;
	ADS124S08_spi_rx_buffer[1] = 0x00;
	ADS124S08_spi_rx_buffer[2] = 0x00;

	ADS124S08_buf_tx.buf = &ADS124S08_spi_tx_buffer;
	ADS124S08_buf_tx.len = 3;
	ADS124S08_buf_set_tx.buffers = &ADS124S08_buf_tx;
	ADS124S08_buf_set_tx.count = 1;

	ADS124S08_buf_rx.buf = &ADS124S08_spi_rx_buffer;
	ADS124S08_buf_rx.len = 3;
	ADS124S08_buf_set_rx.buffers = &ADS124S08_buf_rx;
	ADS124S08_buf_set_rx.count = 1;

	ADS124S08_select();
	int err = spi_transceive(ADS124S08_spi_dev, &ADS124S08_config, &ADS124S08_buf_set_tx, &ADS124S08_buf_set_rx);
	if (err)
	{
		LOG_ERR("ADS124S08_regRead failed. Writing to register: %02X", ADS124S08_spi_tx_buffer[0]);
	}
	ADS124S08_release();

	LOG_INF("ADS124S08_regRead tx: %02x %02x %02x", ADS124S08_spi_tx_buffer[0],
			ADS124S08_spi_tx_buffer[1],
			ADS124S08_spi_tx_buffer[2]);
	LOG_INF("ADS124S08_regRead rx: %02x %02x %02x", ADS124S08_spi_rx_buffer[0],
			ADS124S08_spi_rx_buffer[1],
			ADS124S08_spi_rx_buffer[2]);

	return ADS124S08_spi_rx_buffer[2];
}

//
//  Reads a group of registers starting at the specified address
//
//  \param regnum is addr_mask 8-bit mask of the register from which we start reading
//  \param count The number of registers we wish to read
//  \param *location pointer to the location in memory to write the data
//
void ADS124S08_readRegs(unsigned int regnum, unsigned int count, uint8_t *data)
{
	ADS124S08_spi_tx_buffer[0] = REGRD_OPCODE_MASK + (regnum & 0x1f);
	ADS124S08_spi_tx_buffer[1] = count - 1;

	ADS124S08_buf_tx.buf = &ADS124S08_spi_tx_buffer;
	ADS124S08_buf_tx.len = 2;
	ADS124S08_buf_set_tx.buffers = &ADS124S08_buf_tx;
	ADS124S08_buf_set_tx.count = 1;

	ADS124S08_buf_rx.buf = &ADS124S08_spi_rx_buffer;
	ADS124S08_buf_rx.len = 2;
	ADS124S08_buf_set_rx.buffers = &ADS124S08_buf_rx;
	ADS124S08_buf_set_rx.count = 1;

	ADS124S08_select();
	int err = spi_transceive(ADS124S08_spi_dev, &ADS124S08_config, &ADS124S08_buf_set_tx, &ADS124S08_buf_set_rx);
	if (err)
	{
		LOG_ERR("ADS124S08_readRegs failed (1). Reading registers.");
	}

	memset(&ADS124S08_spi_tx_buffer[0], 0, SPI_BUF_SIZE);

	ADS124S08_buf_tx.len = count;
	ADS124S08_buf_rx.len = count;

	err = spi_transceive(ADS124S08_spi_dev, &ADS124S08_config, &ADS124S08_buf_set_tx, &ADS124S08_buf_set_rx);
	if (err)
	{
		LOG_ERR("ADS124S08_readRegs failed (2). Reading registers.");
	}

	ADS124S08_release();

	for (int i = 0; i < count; i++)
	{
		printf("ADS124S08_regRead reg: %d: value: %d\n", i, (int)ADS124S08_spi_rx_buffer[i]);
		k_sleep(100);
		data[i] = ADS124S08_spi_rx_buffer[i];
	}
}

//
//  Writes a single of register with the specified data
//
//  \param regnum addr_mask 8-bit mask of the register to which we start writing
//  \param data to be written
//
void ADS124S08_regWrite(unsigned int regnum, unsigned char data)
{
	ADS124S08_spi_tx_buffer[0] = REGWR_OPCODE_MASK + (regnum & 0x1f);
	ADS124S08_spi_tx_buffer[1] = 0x00;
	ADS124S08_spi_tx_buffer[2] = data;

	ADS124S08_buf_tx.buf = &ADS124S08_spi_tx_buffer;
	ADS124S08_buf_tx.len = 3;
	ADS124S08_buf_set_tx.buffers = &ADS124S08_buf_tx;
	ADS124S08_buf_set_tx.count = 1;

	ADS124S08_select();
	int err = spi_write(ADS124S08_spi_dev, &ADS124S08_config, &ADS124S08_buf_set_tx);
	if (err)
	{
		LOG_ERR("ADS124S08_regWrite. Writing to register: %02X value: %d", regnum, data);
	}
	ADS124S08_release();
}

//  Sends a command to the ADS124S08
//
//  \param op_code is the command being issued
//
void ADS124S08_sendCommand(uint8_t op_code)
{
	LOG_DBG("ADS124S08_sendCommand");

	ADS124S08_spi_tx_buffer[0] = op_code;
	ADS124S08_spi_rx_buffer[0] = 0x00;

	ADS124S08_buf_tx.buf = &ADS124S08_spi_tx_buffer;
	ADS124S08_buf_tx.len = 1;
	ADS124S08_buf_set_tx.buffers = &ADS124S08_buf_tx;
	ADS124S08_buf_set_tx.count = 1;

	ADS124S08_buf_rx.buf = &ADS124S08_spi_rx_buffer;
	ADS124S08_buf_rx.len = 1;
	ADS124S08_buf_set_rx.buffers = &ADS124S08_buf_rx;
	ADS124S08_buf_set_rx.count = 1;

	ADS124S08_select();

	int err = spi_transceive(ADS124S08_spi_dev, &ADS124S08_config, &ADS124S08_buf_set_tx, &ADS124S08_buf_set_rx);
	if (err)
	{
		LOG_ERR("ADS124S08__sendCommand failed. opcode: %02X", op_code);
	}

	ADS124S08_release();
}

//  Sends a STOP/START command sequence to the ADS124S08 to restart conversions (SYNC)
//
void ADS124S08_reStart(void)
{
	ADS124S08_sendCommand(STOP_OPCODE_MASK);
	ADS124S08_sendCommand(START_OPCODE_MASK);
	return;
}

//  Sets the GPIO hardware START pin high (red LED)
//
void ADS124S08_startConversion()
{
	digitalWrite(START_PIN, HIGH);
	k_sleep(2);
}

//  Sets the GPIO hardware START pin low
//
void ADS124S08_stopConversion()
{
	digitalWrite(START_PIN, LOW);
	k_sleep(2);
}

void ADS124S08_hardReset()
{
	gpio_pin_write(get_GPIO_device(), ADC_RESET, 0);
	k_sleep(200);
	gpio_pin_write(get_GPIO_device(), ADC_RESET, 1);
	k_sleep(200);
}

//  Read the last conversion result
//
unsigned int ADS124S08_dataRead(uint8_t *status, uint8_t *crc)
{
	uint8_t xcrc;
	uint8_t xstatus;
	unsigned int iData;
	ADS124S08_select();

	ADS124S08_spi_tx_buffer[0] = 0x00;
	ADS124S08_spi_rx_buffer[0] = 0x00;
	ADS124S08_buf_tx.buf = &ADS124S08_spi_tx_buffer;
	ADS124S08_buf_tx.len = 1;
	ADS124S08_buf_set_tx.buffers = &ADS124S08_buf_tx;
	ADS124S08_buf_set_tx.count = 1;

	ADS124S08_buf_rx.buf = &ADS124S08_spi_rx_buffer;
	ADS124S08_buf_rx.len = 1;
	ADS124S08_buf_set_rx.buffers = &ADS124S08_buf_rx;
	ADS124S08_buf_set_rx.count = 1;

	int err = spi_transceive(ADS124S08_spi_dev, &ADS124S08_config, &ADS124S08_buf_set_tx, &ADS124S08_buf_set_rx);
	if (err)
	{
		LOG_ERR("ADS124S08_dataRead failed (conversion 1).");
	}
	xstatus = ADS124S08_spi_rx_buffer[0];
	// LOG_INF("ADS124S08_dataRead xstatus: %d", xstatus);
	status[0] = (uint8_t)xstatus;

	ADS124S08_spi_tx_buffer[0] = 0x00;
	ADS124S08_spi_tx_buffer[1] = 0x00;
	ADS124S08_spi_tx_buffer[2] = 0x00;
	ADS124S08_spi_rx_buffer[0] = 0x00;
	ADS124S08_spi_rx_buffer[1] = 0x00;
	ADS124S08_spi_rx_buffer[2] = 0x00;
	ADS124S08_buf_tx.buf = &ADS124S08_spi_tx_buffer;
	ADS124S08_buf_tx.len = 3;
	ADS124S08_buf_set_tx.buffers = &ADS124S08_buf_tx;
	ADS124S08_buf_set_tx.count = 1;

	ADS124S08_buf_rx.buf = &ADS124S08_spi_rx_buffer;
	ADS124S08_buf_rx.len = 3;
	ADS124S08_buf_set_rx.buffers = &ADS124S08_buf_rx;
	ADS124S08_buf_set_rx.count = 1;

	err = spi_transceive(ADS124S08_spi_dev, &ADS124S08_config, &ADS124S08_buf_set_tx, &ADS124S08_buf_set_rx);
	if (err)
	{
		LOG_ERR("ADS124S08_dataRead failed (conversion 2).");
	}

	// get the conversion data (3 bytes)
	uint8_t data[3];
	data[0] = ADS124S08_spi_rx_buffer[0];
	data[1] = ADS124S08_spi_rx_buffer[1];
	data[2] = ADS124S08_spi_rx_buffer[2];

	iData = data[0];
	iData = (iData << 8) + data[1];
	iData = (iData << 8) + data[2];

	// Read the CRC
	ADS124S08_spi_tx_buffer[0] = 0x00;
	ADS124S08_spi_rx_buffer[0] = 0x00;
	ADS124S08_buf_tx.buf = &ADS124S08_spi_tx_buffer;
	ADS124S08_buf_tx.len = 1;
	ADS124S08_buf_set_tx.buffers = &ADS124S08_buf_tx;
	ADS124S08_buf_set_tx.count = 1;

	ADS124S08_buf_rx.buf = &ADS124S08_spi_rx_buffer;
	ADS124S08_buf_rx.len = 1;
	ADS124S08_buf_set_rx.buffers = &ADS124S08_buf_rx;
	ADS124S08_buf_set_rx.count = 1;

	err = spi_transceive(ADS124S08_spi_dev, &ADS124S08_config, &ADS124S08_buf_set_tx, &ADS124S08_buf_set_rx);
	if (err)
	{
		LOG_ERR("ADS124S08_dataRead failed (conversion 3).");
	}
	xcrc = ADS124S08_spi_rx_buffer[0];
	crc[0] = (uint8_t)xcrc;

	ADS124S08_release();
	return iData;
}

void dumpRegisters(void)
{
	unsigned int index;
	uint8_t registers[18];
	ADS124S08_readRegs(0, 18, registers);
	printf("Register Contents\n");
	printf("-----------------\n");

	for (index = 0; index < 18; index++)
	{
		printf("Register %02X = %02X\n", index, registers[index]);
		k_sleep(100);
	}
}

void configureAdc(unsigned char channel)
{
	ADS124S08_sendCommand(WAKE_OPCODE_MASK);
	ADS124S08_regWrite(INPMUX_ADDR_MASK, channel | ADS_N_AINCOM);
	ADS124S08_regWrite(PGA_ADDR_MASK, 0b00000000);
	ADS124S08_regWrite(REF_ADDR_MASK, 0b00110000);
	ADS124S08_regWrite(SYS_ADDR_MASK, 0b00000011);
	ADS124S08_reStart();
	k_sleep(10);
}

unsigned int sampleChannel(unsigned char channel)
{
	uint8_t status = 0;
	uint8_t crc = 0;

	configureAdc(channel);
	k_sleep(100);

	ADS124S08_startConversion();
	unsigned int data = ADS124S08_dataRead(&status, &crc);
	ADS124S08_stopConversion();
	bool valid = ~(status & DEVICE_READY_FLAG);
	if (!valid)
	{
		LOG_ERR("ADC not ready while sampling channel: %c", channel);
	}
	//		 printf("ADC (2's): %u , status :%d, crc: %d - valid: %s\n", data, status, crc, (valid ? "YES" : "NO"));

	return data;
}

void ADC_main(void *foo, void *bar, void *gazonk)
{
	// LOG_INF("Initializing ADS124S08...\n");
	// configureAdc(ADS_P_AIN5);
	// dumpRegisters();
	// float LSB = 0.0000005960464478;

	sensor_node_message.afe3_sample.op1 = sampleChannel(ADS_P_AIN0);
	sensor_node_message.afe3_sample.op2 = sampleChannel(ADS_P_AIN1);
	sensor_node_message.afe3_sample.op3 = sampleChannel(ADS_P_AIN2);
	sensor_node_message.afe3_sample.op4 = sampleChannel(ADS_P_AIN3);
	sensor_node_message.afe3_sample.op5 = sampleChannel(ADS_P_AIN4);
	sensor_node_message.afe3_sample.op6 = sampleChannel(ADS_P_AIN5);
	sensor_node_message.afe3_sample.pt = sampleChannel(ADS_P_AIN8);
}