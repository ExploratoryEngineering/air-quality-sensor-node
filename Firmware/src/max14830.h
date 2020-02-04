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

#define CLOCK_FREQUENCY 3686400

#define LCR_REGISTER 0x0B
#define PLLCONFIG_REGISTER 0x1A
#define BRGCONFIG_REGISTER 0x1B
#define DIVLSB_REGISTER 0x1C
#define DIVMSB_REGISTER 0x1D
#define CLKSOURCE_REGISTER 0x1E
#define RxFIFOLvl_REGISTER 0x12
#define TxFIFOLvl_REGISTER 0x11
#define TxSYNCH_REGISTER 0x20
#define TxCOMMAND4 0xE0
#define TxCOMMAND8 0xE1
#define GLOBLCOMND_REGISTER 0x1F
#define MODE1_REGISTER 0x09
#define MODE2_REGISTER 0x0A
#define RXTIMEOUT_REGISTER 0x0C
#define GLOBAL_IRQ_REGISTER 0x1F


#define MODE2XBIT 0b00010000
#define MODE4XBIT 0b00100000

#define IRQENABLE_REGISTER 0x01
#define RFifoTrgIEn (1 << 3)
#define LSRErrIEn (1 << 0)

#define INTERRUPT_STATUS_REGISTER 0x02
#define LSRErrInt (1 << 0)
#define SpCharInt (1 << 1)
#define STSInt (1 << 2)
#define RFifoTrigInt (1 << 3)
#define TFifoTrigInt (1 << 4)
#define TFifoEmptyInt (1 << 5)
#define RFifoEmptyInt (1 << 6)
#define CTSInt (1 << 7)

#define IRQSel (1 << 7)

#define FIFOTRIGLVL_REGISTER 0x10
#define LSRINTEN_REGISTER 0x03
#define LSR_REGISTER 0x04

#define I2C_NUM 1

#define THR_REGISTER 0x00
#define RHR_REGISTER 0x00

#define NO_PARITY 0
#define EVEN_PARITY 1
#define ODD_PARITY 2

#define DATA_READY 0
#define RTimeout 0b00000001


typedef void (*max_char_callback_t)(uint8_t data);

void init_max14830();
void initUart(uint8_t address, int baudrate, uint8_t wordlength, uint8_t parity, uint8_t stopBits);
int max14830_write(uint8_t address, uint8_t reg, uint8_t data);
uint8_t max14830_read(uint8_t address, uint8_t reg);
int sendMessage(uint8_t address, const uint8_t * txBuffer, uint8_t txLength);
void MAX_init(max_char_callback_t cb);

#endif

