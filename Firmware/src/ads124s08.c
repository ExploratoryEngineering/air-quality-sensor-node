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

/*
 * This class is based on the ADS124S08.c provided by TI and adapted for use with
 * Particle Photon and other Arduino-like MCU's.
 *
 * Note that the confusingly named "selectDeviceCSLow"
 * actually selects the device and that "releaseChipSelect" deselects it. Not obvious
 * from the naming, but according to the datasheet CS is active LOW.
 */

#include <zephyr.h>
#include <spi.h>
#include <device.h>
#include <logging/log.h>
#include "spi_config.h"
#include "ADS124S08.h"
#include "gpio.h"

LOG_MODULE_DECLARE(EE06);

bool ADS124S08_fStart;
// void ADS124S08_DRDY_int(void);
uint8_t ADS124S08__drdy_pin;
uint8_t ADS124S08__start_pin;
uint8_t ADS124S08__reset_pin;
struct spi_config ADS124S08_config;
struct spi_cs_control ADS124S08_control;
struct spi_buf ADS124S08_buf_tx;
struct spi_buf_set ADS124S08_buf_set_tx;
struct spi_buf ADS124S08_buf_rx;
struct spi_buf_set ADS124S08_buf_set_rx;
struct device * ADS124S08_spi_dev;

u8_t ADS124S08_spi_tx_buffer[SPI_BUF_SIZE];
u8_t ADS124S08_spi_rx_buffer[SPI_BUF_SIZE];

uint8_t ADS124S08_registers[NUM_REGISTERS];
bool ADS124S08_converting;

/*
 * Writes the nCS pin low and waits a while for the device to finish working before
 * handing control back to the caller for a SPI transfer.
 */
void ADS124S08_selectDeviceCSLow(void){
	digitalWrite( CS_PIN, LOW );
}

/*
 * Pulls the nCS pin high. Performs no waiting.
 */
void ADS124S08_releaseChipSelect(void){
	digitalWrite( CS_PIN, HIGH );
}

/*
 * Initializes device for use in the ADS124S08 EVM.
 *
 * \return True if device is in correct hardware defaults and is connected
 *
 */
void ADS124S08_init(void)
{
	LOG_INF("ADS124S08_init");
    // Moved to gpio.c
	// pinMode( CS_PIN, OUTPUT );
	// pinMode( START_PIN, OUTPUT );
	// pinMode( RESET_PIN, OUTPUT );
	// pinMode( CKEN_PIN, OUTPUT );
	// pinMode( DRDY_PIN, INPUT );

	// digitalWrite( START_PIN, LOW );
	// digitalWrite( RESET_PIN, HIGH );
	// digitalWrite( CKEN_PIN, LOW );

	/* Default register settings */
	ADS124S08_registers[ID_ADDR_MASK] 			= 0x08;
	ADS124S08_registers[STATUS_ADDR_MASK] 	= 0x80;
	ADS124S08_registers[INPMUX_ADDR_MASK]		= 0x01;
	ADS124S08_registers[PGA_ADDR_MASK] 			= 0x00;
	ADS124S08_registers[DATARATE_ADDR_MASK] = 0x14;
	ADS124S08_registers[REF_ADDR_MASK] 			= 0x10;
	ADS124S08_registers[IDACMAG_ADDR_MASK] 	= 0x00;
	ADS124S08_registers[IDACMUX_ADDR_MASK] 	= 0xFF;
	ADS124S08_registers[VBIAS_ADDR_MASK] 		= 0x00;
	ADS124S08_registers[SYS_ADDR_MASK]			= 0x10;
	ADS124S08_registers[OFCAL0_ADDR_MASK] 	= 0x00;
	ADS124S08_registers[OFCAL1_ADDR_MASK] 	= 0x00;
	ADS124S08_registers[OFCAL2_ADDR_MASK] 	= 0x00;
	ADS124S08_registers[FSCAL0_ADDR_MASK] 	= 0x00;
	ADS124S08_registers[FSCAL1_ADDR_MASK] 	= 0x00;
	ADS124S08_registers[FSCAL2_ADDR_MASK] 	= 0x40;
	ADS124S08_registers[GPIODAT_ADDR_MASK] 	= 0x00;
	ADS124S08_registers[GPIOCON_ADDR_MASK]	= 0x00;
	ADS124S08_fStart = false;
	ADS124S08_releaseChipSelect();
	ADS124S08_deassertStart();
}

void ADS124S08_begin()
{
	LOG_DBG("ADS124S08_begin");

	ADS124S08_config.cs = NULL;
	ADS124S08_config.frequency = 1000000;
	ADS124S08_config.operation = SPI_OP_MODE_MASTER | SPI_WORD_SET(8) | SPI_TRANSFER_MSB | SPI_MODE_CPHA;
	ADS124S08_config.slave = 0;
	ADS124S08_spi_dev = device_get_binding(SPI_DEV);
	if (!ADS124S08_spi_dev) {
		LOG_ERR("SPI device driver not found");
	}

	LOG_INF("ADS124S08_config cs: %d", (int)ADS124S08_config.cs);
	LOG_INF("ADS124S08_config frequency: %d", (int)ADS124S08_config.frequency);
	LOG_INF("ADS124S08_config operation: %d", (int)ADS124S08_config.operation);
	LOG_INF("ADS124S08_config slave: %d", (int)ADS124S08_config.slave);
}

/*
 * Reads a single register contents from the specified address
 *
 * \param regnum identifies which address to read
 *
 */
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

    ADS124S08_selectDeviceCSLow();
    int err = spi_transceive(ADS124S08_spi_dev, &ADS124S08_config, &ADS124S08_buf_set_tx, &ADS124S08_buf_set_rx);
    if (err)
    {
      LOG_ERR("ADS124S08_regRead failed. Writing to register: %02X", ADS124S08_spi_tx_buffer[0]);
    }
    ADS124S08_releaseChipSelect();

    LOG_INF("ADS124S08_regRead tx: %02x %02x %02x", ADS124S08_spi_tx_buffer[0], 
                                                    ADS124S08_spi_tx_buffer[1], 
                                                    ADS124S08_spi_tx_buffer[2]);
    LOG_INF("ADS124S08_regRead rx: %02x %02x %02x", ADS124S08_spi_rx_buffer[0], 
                                                    ADS124S08_spi_rx_buffer[1], 
                                                    ADS124S08_spi_rx_buffer[2]);

    return ADS124S08_spi_rx_buffer[2];
}

/*
 * Reads a group of registers starting at the specified address
 *
 * \param regnum is addr_mask 8-bit mask of the register from which we start reading
 * \param count The number of registers we wish to read
 * \param *location pointer to the location in memory to write the data
 *
 */
void ADS124S08_readRegs(unsigned int regnum, unsigned int count, uint8_t *data)
{
 		ADS124S08_spi_tx_buffer[0] = REGRD_OPCODE_MASK + (regnum & 0x1f);
    ADS124S08_spi_tx_buffer[1] = count-1;
  
    ADS124S08_buf_tx.buf = &ADS124S08_spi_tx_buffer;
    ADS124S08_buf_tx.len = 2;
    ADS124S08_buf_set_tx.buffers = &ADS124S08_buf_tx;
    ADS124S08_buf_set_tx.count = 1;

    ADS124S08_buf_rx.buf = &ADS124S08_spi_rx_buffer;
    ADS124S08_buf_rx.len = 2;
    ADS124S08_buf_set_rx.buffers = &ADS124S08_buf_rx;
    ADS124S08_buf_set_rx.count = 1;

    ADS124S08_selectDeviceCSLow();
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

    ADS124S08_releaseChipSelect();

		for (int i=0; i<count; i++)
		{
			 LOG_INF("ADS124S08_regRead reg: %d: value: %d", i, (int)ADS124S08_spi_rx_buffer[i]);
			 data[i] = ADS124S08_spi_rx_buffer[i];
		}
}

/*
 * Writes a single of register with the specified data
 *
 * \param regnum addr_mask 8-bit mask of the register to which we start writing
 * \param data to be written
 *
 */
void ADS124S08_regWrite(unsigned int regnum, unsigned char data)
{
  	ADS124S08_spi_tx_buffer[0] = REGRD_OPCODE_MASK + (regnum & 0x1f);
    ADS124S08_spi_tx_buffer[1] = 0x00;
    ADS124S08_spi_tx_buffer[2] = data;

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

    ADS124S08_selectDeviceCSLow();
    int err = spi_transceive(ADS124S08_spi_dev, &ADS124S08_config, &ADS124S08_buf_set_tx, &ADS124S08_buf_set_rx);
    if (err)
    {
      LOG_ERR("ADS124S08_regWrite. Writing to register: %02X value: %d", regnum, data);
    }
    ADS124S08_releaseChipSelect();
}

/*
 * Writes a group of registers starting at the specified address
 *
 * \param regnum is addr_mask 8-bit mask of the register from which we start writing
 * \param count The number of registers we wish to write
 * \param *location pointer to the location in memory to read the data
 *
 */
void ADS124S08_writeRegs(unsigned int regnum, unsigned int howmuch, unsigned char *data)
{
 		ADS124S08_spi_tx_buffer[0] = REGRD_OPCODE_MASK + (regnum & 0x1f);
    ADS124S08_spi_tx_buffer[1] = howmuch-1;
    ADS124S08_spi_tx_buffer[2] = 0x00;

    ADS124S08_spi_rx_buffer[0] = 0x00;
    ADS124S08_spi_rx_buffer[1] = 0x00;
    ADS124S08_spi_rx_buffer[2] = 0x00;

    ADS124S08_buf_tx.buf = &ADS124S08_spi_tx_buffer;
    ADS124S08_buf_tx.len = 2;
    ADS124S08_buf_set_tx.buffers = &ADS124S08_buf_tx;
    ADS124S08_buf_set_tx.count = 1;

    ADS124S08_buf_rx.buf = &ADS124S08_spi_rx_buffer;
    ADS124S08_buf_rx.len = 2;
    ADS124S08_buf_set_rx.buffers = &ADS124S08_buf_rx;
    ADS124S08_buf_set_rx.count = 1;

    ADS124S08_selectDeviceCSLow();
    int err = spi_transceive(ADS124S08_spi_dev, &ADS124S08_config, &ADS124S08_buf_set_tx, &ADS124S08_buf_set_rx);
    if (err)
    {
      LOG_ERR("ADS124S08_writeRegs failed (1)");
    }

 		ADS124S08_buf_tx.len = howmuch;
		ADS124S08_buf_rx.len = howmuch;
		for(int i=0; i < howmuch; i++)
		{
			ADS124S08_spi_tx_buffer[i] = data[i];
		}

		err = spi_transceive(ADS124S08_spi_dev, &ADS124S08_config , &ADS124S08_buf_set_tx, &ADS124S08_buf_set_rx);
    if (err)
    {
      LOG_ERR("ADS124S08_writeRegs failed (2)");
    }

    ADS124S08_releaseChipSelect();
}

/*
 * Sends a command to the ADS124S08
 *
 * \param op_code is the command being issued
 *
 */
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

	ADS124S08_selectDeviceCSLow();

	int err = spi_transceive(ADS124S08_spi_dev, &ADS124S08_config, &ADS124S08_buf_set_tx, &ADS124S08_buf_set_rx);
	if (err)
	{
		LOG_ERR("ADS124S08__sendCommand failed. opcode: %02X", op_code);
	}

	ADS124S08_releaseChipSelect();
}

/*
 * Sends a STOP/START command sequence to the ADS124S08 to restart conversions (SYNC)
 *
 */
void ADS124S08_reStart(void)
{
	ADS124S08_sendCommand(STOP_OPCODE_MASK);
	ADS124S08_sendCommand(START_OPCODE_MASK);
	return;
}

/*
 * Sets the GPIO hardware START pin high (red LED)
 *
 */
void ADS124S08_assertStart()
{
	ADS124S08_fStart = true;
	digitalWrite(START_PIN ,HIGH);
}

/*
 * Sets the GPIO hardware START pin low
 *
 */
void ADS124S08_deassertStart()
{
	ADS124S08_fStart = false;
	digitalWrite(START_PIN, LOW);
}

/*
 * Sets the GPIO hardware external oscillator enable pin high
 *
 */
void ADS124S08_assertClock()
{
	digitalWrite(CKEN_PIN, 1);
}

/*
 * Sets the GPIO hardware external oscillator enable pin low
 *
 */
void ADS124S08_deassertClock()
{
	digitalWrite(CKEN_PIN, LOW);
}

int ADS124S08_rData(uint8_t *dStatus, uint8_t *dData, uint8_t *dCRC)
{
	int result = -1;
	ADS124S08_selectDeviceCSLow();

	// according to datasheet chapter 9.5.4.2 Read Data by RDATA Command
	ADS124S08_sendCommand(RDATA_OPCODE_MASK);

	// if the Status byte is set - grab it
	uint8_t shouldWeReceiveTheStatusByte = (ADS124S08_registers[SYS_ADDR_MASK] & 0x01) == DATA_MODE_STATUS;
	if( shouldWeReceiveTheStatusByte )
	{
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
			LOG_ERR("ADS124S08_rData failed.");
		}

		dStatus[0] = ADS124S08_spi_rx_buffer[0];
		LOG_INF("ADS124S08_rData. Status byte: %02X", dStatus[0]);
	}

	ADS124S08_spi_tx_buffer[0] = 0x00;
	ADS124S08_spi_tx_buffer[1] = 0x00;
	ADS124S08_spi_tx_buffer[2] = 0x00;

	ADS124S08_spi_rx_buffer[0] = 0x00;
	ADS124S08_spi_rx_buffer[1] = 0x00;
	ADS124S08_spi_rx_buffer[2] = 0x00;

	ADS124S08_buf_tx.len = 3;
	ADS124S08_buf_rx.len = 3;

	int err = spi_transceive(ADS124S08_spi_dev, &ADS124S08_config, &ADS124S08_buf_set_tx, &ADS124S08_buf_set_rx);
	if (err)
	{
		LOG_ERR("ADS124S08_rData failed (conversion).");
	}
	
	// get the conversion data (3 bytes)
	uint8_t data[3];
	data[0] = ADS124S08_spi_rx_buffer[0];
	data[1] = ADS124S08_spi_rx_buffer[1];
	data[2] = ADS124S08_spi_rx_buffer[2];
	result = data[0];
	result = (result<<8) + data[1];
	result = (result<<8) + data[2];
	//Serial.printlnf(" 1: %02x 2: %02x, 3: %02x = %d", data[0], data[1], data[2], result);

	// is CRC enabled?
	uint8_t isCrcEnabled = (ADS124S08_registers[SYS_ADDR_MASK] & 0x02) == DATA_MODE_CRC;
	if( isCrcEnabled )
	{
		ADS124S08_spi_tx_buffer[0] = 0x00;
		ADS124S08_spi_rx_buffer[0] = 0x00;

		ADS124S08_buf_tx.len = 1;
		ADS124S08_buf_rx.len = 1;

		int err = spi_transceive(ADS124S08_spi_dev, &ADS124S08_config, &ADS124S08_buf_set_tx, &ADS124S08_buf_set_rx);
		if (err)
		{
			LOG_ERR("ADS124S08_rData failed (conversion).");
		}
		dCRC[0] = ADS124S08_spi_rx_buffer[0];
	}

	ADS124S08_releaseChipSelect();
	return result;
}

/*
 *
 * Read the last conversion result
 *
 */
int ADS124S08_dataRead(uint8_t *dStatus, uint8_t *dData, uint8_t *dCRC)
{
	uint8_t xcrc;
	uint8_t xstatus;
	int iData;
	ADS124S08_selectDeviceCSLow();
	if((ADS124S08_registers[SYS_ADDR_MASK] & 0x01) == DATA_MODE_STATUS)
	{
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
		LOG_INF("ADS124S08_dataRead xstatus: %d", xstatus);
		
		dStatus[0] = (uint8_t)xstatus;
	}

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

	int err = spi_transceive(ADS124S08_spi_dev, &ADS124S08_config, &ADS124S08_buf_set_tx, &ADS124S08_buf_set_rx);
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
	iData = (iData<<8) + data[1];
	iData = (iData<<8) + data[2];
	if((ADS124S08_registers[SYS_ADDR_MASK] & 0x02) == DATA_MODE_CRC)
	{

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
			LOG_ERR("ADS124S08_dataRead failed (conversion 3).");
		}
		xcrc = ADS124S08_spi_rx_buffer[0];
		dCRC[0] = (uint8_t)xcrc;
	}
	ADS124S08_releaseChipSelect();
	return iData ;
}