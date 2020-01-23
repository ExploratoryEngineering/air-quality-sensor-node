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

// The MAX14830 datasheet is available from https://datasheets.maximintegrated.com/en/ds/MAX14830.pdf

#include "max14830.h"

#include <zephyr.h>
#include <i2c.h>
#include "i2c_config.h"
#include <logging/log.h>
#include <gpio.h>
#include "gpio.h"
#include "pinout.h"

extern struct device * gpio_device;

extern struct k_sem uart_semaphore;

bool dataReady = false;

#define LOG_LEVEL CONFIG_EE06_LOG_LEVEL
LOG_MODULE_DECLARE(EE06);


#define RX_BUFFER_SIZE 256

uint8_t RX_BUFFER[RX_BUFFER_SIZE];

// static struct gpio_callback gpio_cb;

int rxIndex = 0;

int max14830_write(uint8_t address, uint8_t reg, uint8_t data)
{
    uint8_t txBuffer[] = {reg, data};
    int err = i2c_write(get_I2C_device(), txBuffer, 2, EE_NBIOT_01_ADDRESS);
    if (0 != err)
    {
        LOG_ERR("i2c_write failed with error: %d", err);
    }
    return err;
}

uint8_t max14830_read(uint8_t address, uint8_t reg)
{
    uint8_t txBuffer[] = {reg};
    uint8_t rxBuffer[] = {0};

    int err = i2c_write_read(get_I2C_device(), EE_NBIOT_01_ADDRESS, txBuffer, 1, rxBuffer, 1);
    if (0 != err)
    {
        LOG_ERR("i2c_write_read failed with error: %d", err);
    }

    return rxBuffer[0];
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
    LOG_INF("MAX14830: Base clock frequency: %d, Address: %02X, UART baudrate: %d, divisor: %d, mode: %d\n", CLOCK_FREQUENCY, address, baud, div, mode);

    max14830_write(address, DIVLSB_REGISTER, div / 16);
    max14830_write(address, DIVMSB_REGISTER, (div / 16) >> 8);
    max14830_write(address, BRGCONFIG_REGISTER, (div % 16) | mode);

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
    LOG_INF("MAX14830: Waiting for reset signal...\n");
    u32_t resetVal;
    do {
        LOG_INF("MAX14830: Waiting...\n");
        k_sleep(1);
        gpio_pin_read(get_GPIO_device(), MAX14830_IRQ, &resetVal);
    } while (0 == resetVal);
}

void EnableRxMode(uint8_t address)
{
    uint8_t mode1 = max14830_read(address, MODE1_REGISTER);
    mode1 |= 0b00000010;
    max14830_write(address, MODE1_REGISTER, mode1);
}

void EnableTxMode(uint8_t address)
{
    uint8_t mode1 = max14830_read(address, MODE1_REGISTER);
    mode1 &= 0b11111101;
    max14830_write(address, MODE1_REGISTER, mode1);
}

void WaitForTx(uint8_t address)
{
     while (max14830_read(address, TxFIFOLvl_REGISTER));
}

// void WaitForRx(uint8_t address)
// {
//     int ret;
//     do {
//         ret = max14830_read(address, RxFIFOLvl_REGISTER);
//     } while (0 == ret);
// }

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
    memset(RX_BUFFER, 0, RX_BUFFER_SIZE);    
}


int sendMessage(uint8_t address, uint8_t * txBuffer, uint8_t txLength)
{
    if (0 != k_sem_take(&uart_semaphore, K_MSEC(100)))
    {
        LOG_ERR("(sendMessage) Uart not available");
        return -1;
    }

    flushRXBuffer();
    DiscardWaitingRxJunk(address);
    EnableTxMode(address);

    for (int i=0; i<txLength; i++) {
        max14830_write(EE_NBIOT_01_ADDRESS, THR_REGISTER, *txBuffer++);
    }

    WaitForTx(address);
    EnableRxMode(address);

    k_sem_give(&uart_semaphore);
    return 0;
}


void readFromRxFifo(uint8_t address) 
{
    uint8_t fifo_level = 0;
    uint8_t ch;
    while (true) 
    {
        fifo_level = max14830_read(address, RxFIFOLvl_REGISTER);
        if (0 == fifo_level) 
        {
            break;
        }
        ch = max14830_read(address, RHR_REGISTER);

        printk("%c ", ch);
        if ('\r' == ch) 
        {
            printk("\n");
            // TODO: Copy RX_BUFFER into response message and flushrx
        }
        k_sleep(1);

        RX_BUFFER[rxIndex] = ch;
        RX_BUFFER[rxIndex+1] = 0;
        rxIndex++;
        
        if (rxIndex > RX_BUFFER_SIZE-1)
        {
            rxIndex = 0;
            LOG_ERR("Woops. Slight case of buffer overrun here...\n");
            break;
        }
    } 

    printk("\n");
    k_sleep(1);

}

void readReply()
{
    if (0 != k_sem_take(&uart_semaphore, K_MSEC(100)))
    {
        LOG_ERR("(readReply) Uart not available");
        return;
    }

    // The trigger can be read from any UART 
    uint8_t uart_trigger = max14830_read(EE_NBIOT_01_ADDRESS  , GLOBAL_IRQ_REGISTER); 

    uint8_t uart_address = 0;
    uart_trigger &= 0x0F;
    if (uart_trigger == 0x0F)
        return;
    // The UART multiplexer supports up to 4 channels (so far, we're only using UART0)
    if ((uart_trigger & 0b0001) == 0) 
    {
        uart_address = EE_NBIOT_01_ADDRESS; 
    } 
    // else {
    //     // uart_address = <other device>
    // }

    // Check interrupt cause. 
    uint8_t cause = max14830_read(uart_address, INTERRUPT_STATUS_REGISTER);
    if (cause & LSRErrInt) 
    {
        printk("Cause: %02X\n", cause);
        k_sleep(100);
        // Check the line status register
        uint8_t line_status = max14830_read(uart_address, LSR_REGISTER);
        if ((line_status & RTimeout) || (line_status & RFifoTrigInt))
        {
            readFromRxFifo(uart_address);
        }
    }
    k_sem_give(&uart_semaphore);
}

// void irq_handler(struct device *gpiob, struct gpio_callback *cb, u32_t pins)
// {
//     dataReady = true;
// }

void EnableRxFIFOIrq(uint8_t address)
{   
//     gpio_device = get_GPIO_device();
//     int	ret = gpio_pin_configure(gpio_device, MAX14830_IRQ, GPIO_INT | GPIO_PUD_PULL_UP | GPIO_INT_EDGE | GPIO_INT_ACTIVE_LOW | GPIO_DIR_IN);
//     if (ret) {
// 		printk("Error configuring %d!\n", MAX14830_IRQ);
// 	}
// 	gpio_init_callback(&gpio_cb, irq_handler, BIT(MAX14830_IRQ));
// 	ret = gpio_add_callback(gpio_device, &gpio_cb);
//     if (ret) {
// 		printk("Error enabling callback %d!\n", MAX14830_IRQ);
// 	}
// 	ret = gpio_pin_enable_callback(gpio_device, MAX14830_IRQ);
//     if (ret) {
// 		printk("Error enabling callback %d!\n", MAX14830_IRQ);
// 	}

    max14830_write(address, IRQENABLE_REGISTER, RFifoTrgIEn | LSRErrIEn);
    max14830_write(address, FIFOTRIGLVL_REGISTER, (1 << 4));
    max14830_write(address, MODE1_REGISTER, IRQSel);

    // Enable RTimeout interrupt
    max14830_write(address, LSRINTEN_REGISTER, 0b00001111);
}


void MAX_entry_point(void * foo, void * bar, void * gazonk)
{
    LOG_INF("Initializing MAX14830...\n");
    resetWait();
    
    // Initialize baud rate, parity, word length and stop bits for each uart
    initUart(EE_NBIOT_01_ADDRESS, 9600, 8, NO_PARITY, 1);
    EnableRxFIFOIrq(EE_NBIOT_01_ADDRESS);
    max14830_read(EE_NBIOT_01_ADDRESS, INTERRUPT_STATUS_REGISTER); // What the actual .... ?

    flushRXBuffer();

    // For the time being, we are just polling the 14830 IRQ pin
    u32_t irq_status = -1;
    while (true) 
    {
       printk("Waiting for rx...\n");
       k_sleep(200);

        int ret = gpio_pin_read(get_GPIO_device(), MAX14830_IRQ, &irq_status);
        if (0 == ret)
        {
            if (DATA_READY == irq_status) 
            {
                 readReply();
            }
        }
        k_sleep(100);
    }
}
