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

/*
#include "max14830.h"

#include <zephyr.h>


extern uint8_t TX_BUFFER[TX_BUFFER_SIZE];
extern uint8_t RX_BUFFER[RX_BUFFER_SIZE];

int rxIndex = 0;

void max14830_write(uint8_t address, uint8_t reg, uint8_t data)
{
    int rc;
    uint8_t data_txBuffer[2] = {reg, data};
    struct hal_i2c_master_data txData = {
        .len = sizeof(data_txBuffer),
        .address = address,
        .buffer = data_txBuffer,
    };
    rc = hal_i2c_master_write(1, &txData, 1, 1);
    if (rc != 0)
    {
        console_printf("   ERROR: I2C write failed! (register: %02X, data: %02X\n", address, data);
    }
}

uint8_t max14830_read(uint8_t address, uint8_t reg)
{
    uint8_t data_txBuffer[] = {reg};
    struct hal_i2c_master_data txData = {
        .len = sizeof(data_txBuffer),
        .address = address,
        .buffer = data_txBuffer,
    };
    hal_i2c_master_write(1, &txData, 1, 0);

    uint8_t data_rxBuffer[] = {0};
    struct hal_i2c_master_data rxData = {
        .len = sizeof(data_rxBuffer),
        .address = address,
        .buffer = data_rxBuffer,
    };
    hal_i2c_master_read(1, &rxData, 1, 1);

    return data_rxBuffer[0];
}


void initUart(uint8_t address, int baud, uint8_t wordlength, uint8_t parity, uint8_t stopBits)
{
    unsigned int mode = 0, clk = CLOCK_FREQUENCY, div = clk / baud;

	// Check for minimal value for divider
	if (div < 16)
		div = 16;

	if (clk % baud && (div / 16) < 0x8000) {
		// Mode x2 
		mode = MODE2XBIT;
		clk = CLOCK_FREQUENCY * 2;
		div = clk / baud;

        // Mode x4
		if (clk % baud && (div / 16) < 0x8000) {
			mode = MODE4XBIT;
			clk = CLOCK_FREQUENCY * 4;
			div = clk / baud;
		}
	}
    console_printf("Base clock frequency: %d, Address: %02X, UART baudrate: %d, divisor: %d, mode: %d\n", CLOCK_FREQUENCY, address, baud, div, mode);

    max14830_write(address, DIVLSB_REGISTER, div / 16);
    max14830_write(address, DIVMSB_REGISTER, (div / 16) >> 8);
    max14830_write(address, BRGCONFIG_REGISTER, (div % 16) | mode);
    //console_printf("LSB : %02X\n", div / 16);
    // console_printf("MSB : %02X\n", (div % 16) | mode);

    uint8_t lcr = 0;

    switch (parity)
    {
        case NO_PARITY:     break;
        case EVEN_PARITY:   lcr |= 0b00011000; 
                            break;
        case ODD_PARITY:    lcr |= 0b00001000; 
                            break;
    }

    switch (wordlength)
    {
        case 5: break;
        case 6: lcr |= 0b00000001;
                break;
        case 7: lcr |= 0b00000010;
                break;
        case 8: lcr |= 0b00000011;
                break;
    }

    if (stopBits != 0)
    {
        lcr &= 0b11111011;
    }

    max14830_write(address, LCR_REGISTER, lcr);
    max14830_write(address, CLKSOURCE_REGISTER, 0b00001010); // PLLBypass

    // Set up rxTimeout
    max14830_write(address, RXTIMEOUT_REGISTER, 0b00000010);
}

void resetWait()
{
    console_printf("MAX14830: Waiting for reset signal...\n");
    while (hal_gpio_read(IRQ_PIN) == 0) {
        console_printf("MAX14830: Waiting...\n");
    }
    console_printf("MAX14830: Reset ok\n");
}

void EnableRxMode(uint8_t address)
{
//    console_printf("Enabling Rx mode\n");
    uint8_t mode1 = max14830_read(address, MODE1_REGISTER);
    mode1 |= 0b00000010;
    max14830_write(address, MODE1_REGISTER, mode1);
}

void EnableTxMode(uint8_t address)
{
//    console_printf("Enabling Tx mode\n");
    uint8_t mode1 = max14830_read(address, MODE1_REGISTER);
    mode1 &= 0b11111101;
    max14830_write(address, MODE1_REGISTER, mode1);
}

void WaitForTx(uint8_t address)
{
     while (max14830_read(address, TxFIFOLvl_REGISTER));
}

void WaitForRx(uint8_t address)
{
     while (max14830_read(address, RxFIFOLvl_REGISTER) == 0);
}

void DiscardWaitingRxJunk(uint8_t address)
{
    uint8_t fifo = 0;
    do {
        fifo = max14830_read(address, RxFIFOLvl_REGISTER);
        max14830_read(address, RHR_REGISTER);
    }
    while (fifo != 0);
}

void flushRXBuffer()
{
    rxIndex = 0;
    RX_BUFFER[0] = 0;
    memset(RX_BUFFER, 0, RX_BUFFER_SIZE);    
}


int sendMessage(uint8_t address, uint8_t * txBuffer, uint8_t * rxBuffer)
{
    flushRXBuffer();
    DiscardWaitingRxJunk(address);
    int txLength = strlen((char*)txBuffer);

    EnableTxMode(address);

    if (txLength+2 > TX_BUFFER_SIZE)
    {
        console_printf("Woops. TxBuffer overrun");
        return -1;
    }

    TX_BUFFER[0] = THR_REGISTER;
    for (int i=0; i<txLength; i++)
    {
        TX_BUFFER[i+1] = *txBuffer;    
        txBuffer++;
    }
    TX_BUFFER[txLength+1] = 0;

    //console_printf("Sending message :%s (length: %d)\n",(char *)(&TX_BUFFER[1]), strlen((char *)(&TX_BUFFER[1])));

    struct hal_i2c_master_data txData = {
        .len = txLength+1,
        .address = address,
        .buffer = TX_BUFFER,
    };
    int rc = hal_i2c_master_write(1, &txData, 1, 1);
    if (rc != 0)
    {
        console_printf("   ERROR: I2C write failed! (address: %02X)\n", address);
        return -1;
    }
    
    WaitForTx(address);
    EnableRxMode(address);
    WaitForRx(address);

    // We can do this, in order to fake synchronicity, or we can implement a full fledged event driven command / response stack thingy...
    os_time_delay(OS_TICKS_PER_SEC*2); 
    return 0;
}


void readFromRxFifo(uint8_t address) 
{
    uint8_t fifo = 0;
    uint8_t ch;
    do {
        fifo = max14830_read(address, RxFIFOLvl_REGISTER);
        ch = max14830_read(address, RHR_REGISTER);
        // console_printf(" %c ", ch);
        RX_BUFFER[rxIndex] = ch;
        RX_BUFFER[rxIndex+1] = 0;
        rxIndex++;
        
        if (rxIndex > RX_BUFFER_SIZE-1)
        {
            rxIndex = 0;
            console_printf("Woops. Slight case of buffer overrun here...\n");
            break;
        }
    } while (fifo > 0);
}


static void irq_handler()
{
    uint8_t uart_trigger = max14830_read(EE_NBIOT_01_ADDRESS  , GLOBAL_IRQ_REGISTER); // can be read from any UART 
    uint8_t uart_address = 0;
    uart_trigger &= 0x0F;
    if (uart_trigger == 0x0F)
        return;
    if ((uart_trigger & 0b0001) == 0)
    {
        uart_address = EE_NBIOT_01_ADDRESS; 
        //console_printf("Interrupt from EE-NBIOT-01\n");
    } else if ((uart_trigger & 0b0010) == 0) {
        uart_address = HONEYWELL_ADDRESS; 
        //console_printf("Interrupt from Honeywell\n");
    }

    uint8_t cause = max14830_read(uart_address, INTERRUPT_STATUS_REGISTER);
    if (cause & LSRErrInt) {
        uint8_t err = max14830_read(uart_address, LSR_REGISTER);
        if (err & 0b00000001) {
            readFromRxFifo(uart_address);
        }
    }
    if (cause & RFifoTrigInt) 
    {
        readFromRxFifo(uart_address);
    }
}

static void EnableRxFIFOIrq(uint8_t address)
{   
    int rc;
    rc = hal_gpio_irq_init(IRQ_PIN, irq_handler, NULL, HAL_GPIO_TRIG_FALLING, HAL_GPIO_PULL_UP);
    if (rc != 0)
    {
        console_printf("hal_gpio_irq_init failed. Error code: %d\n", rc);
    }
    hal_gpio_irq_enable(IRQ_PIN);
    
    max14830_write(address, IRQENABLE_REGISTER, RFifoTrgIEn | LSRErrIEn);
    max14830_write(address, FIFOTRIGLVL_REGISTER, (1 << 4));
    max14830_write(address, MODE1_REGISTER, IRQSel);

    // Enable RTimeout interrupt
    max14830_write(address, LSRINTEN_REGISTER, 0b00001111);
}

void reset_max14830()
{
    hal_gpio_init_out(RESET_PIN, 1);
    hal_gpio_write(RESET_PIN, 0);
    os_time_delay(OS_TICKS_PER_SEC);
    hal_gpio_write(RESET_PIN, 1);
    os_time_delay(OS_TICKS_PER_SEC);    
}

void init_max14830()
{
    console_printf("Initializing MAX14830...\n");

    hal_gpio_init_in(IRQ_PIN, HAL_GPIO_PULL_UP);
    hal_gpio_init_in(RESET_PIN, HAL_GPIO_PULL_UP);

    resetWait();
    
    // Initialize baud rate, parity, word length and stop bits for each uart
    initUart(EE_NBIOT_01_ADDRESS, 9600, 8, NO_PARITY, 1);
    initUart(HONEYWELL_ADDRESS, 9600, 8, NO_PARITY, 1);

    EnableRxFIFOIrq(EE_NBIOT_01_ADDRESS);
    EnableRxFIFOIrq(HONEYWELL_ADDRESS);
    max14830_read(EE_NBIOT_01_ADDRESS, INTERRUPT_STATUS_REGISTER); // What the actual .... ?
    max14830_read(HONEYWELL_ADDRESS, INTERRUPT_STATUS_REGISTER); // What the actual .... ?

}

*/