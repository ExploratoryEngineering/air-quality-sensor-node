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
#include <stdio.h>
#include <gpio.h>
#include "gpio.h"
#include "pinout.h"
#include "init.h"

#define LOG_LEVEL CONFIG_EE06_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(MAX14830);

#define MAX_THREAD_PRIORITY -4
#define MAX_THREAD_STACK_SIZE 1024

struct k_thread max_thread;

K_THREAD_STACK_DEFINE(max_thread_stack, MAX_THREAD_STACK_SIZE);

static struct k_sem rx_sem;
#define RX_SEM_SIZE 128

static max_char_callback_t char_callback;

static struct gpio_callback gpio_cb;

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

    if (clk % baud && (div / 16) < 0x8000)
    {
        // Mode x2
        mode = MODE2XBIT;
        clk = CLOCK_FREQUENCY * 2;
        div = clk / baud;

        // Mode x4
        if (clk % baud && (div / 16) < 0x8000)
        {
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
    case NO_PARITY:
        break;
    case EVEN_PARITY:
        lcr |= 0b00011000;
        break;
    case ODD_PARITY:
        lcr |= 0b00001000;
        break;
    }

    switch (wordlength)
    {
    case 5:
        break;
    case 6:
        lcr |= 0b00000001;
        break;
    case 7:
        lcr |= 0b00000010;
        break;
    case 8:
        lcr |= 0b00000011;
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
    do
    {
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
    while (max14830_read(address, TxFIFOLvl_REGISTER))
        ;
}

void WaitForRx(uint8_t address)
{
    int ret;
    do
    {
        ret = max14830_read(address, RxFIFOLvl_REGISTER);
    } while (0 == ret);
}

void DiscardWaitingRxJunk(uint8_t address)
{
    uint8_t fifo = 0;
    do
    {
        fifo = max14830_read(address, RxFIFOLvl_REGISTER);
        max14830_read(address, RHR_REGISTER);
    } while (fifo != 0);
}

int sendMessage(uint8_t address, const uint8_t *txBuffer, uint8_t txLength)
{
    DiscardWaitingRxJunk(address);
    EnableTxMode(address);

    for (int i = 0; i < txLength; i++)
    {
        max14830_write(address, THR_REGISTER, *txBuffer++);
    }

    WaitForTx(address);
    EnableRxMode(address);

    // // We can do this, in order to fake synchronicity, or we can implement a full fledged event driven command / response stack thingy...

    // (stalehd): )We should fix this :) CoAP and LwM2M requires a slightly quicker send/response
    // thing.
    k_sleep(2000);

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

        if (char_callback) {
            char_callback(ch);
        }
    }
}

void readReply()
{
    // The trigger can be read from any UART
    uint8_t uart_trigger = max14830_read(EE_NBIOT_01_ADDRESS, GLOBAL_IRQ_REGISTER);

    uint8_t uart_address = 0;
    uart_trigger &= 0x0F;
    if (uart_trigger == 0x0F)
        return;
    // The UART multiplexer supports up to 4 channels (so far, we're only using UART0)
    if ((uart_trigger & 0b0001) == 0)
    {
        uart_address = EE_NBIOT_01_ADDRESS;
    }

    // Check interrupt cause.
    uint8_t cause = max14830_read(uart_address, INTERRUPT_STATUS_REGISTER);
    if (cause & LSRErrInt)
    {
        // Check the line status register
        uint8_t line_status = max14830_read(uart_address, LSR_REGISTER);
        if ((line_status & RTimeout) || (line_status & RFifoTrigInt))
        {
            readFromRxFifo(uart_address);
        }
    }
}

void irq_handler(struct device *gpiob, struct gpio_callback *cb, u32_t pins)
{
    k_sem_give(&rx_sem);
}

void EnableRxFIFOIrq(uint8_t address)
{
    struct device *gpio_device = get_GPIO_device();
    int ret = gpio_pin_configure(gpio_device, MAX14830_IRQ, GPIO_INT | GPIO_PUD_PULL_UP | GPIO_INT_EDGE | GPIO_INT_ACTIVE_LOW | GPIO_DIR_IN);
    if (ret)
    {
        LOG_ERR("Error configuring %d!\n", MAX14830_IRQ);
    }
    gpio_init_callback(&gpio_cb, irq_handler, BIT(MAX14830_IRQ));
    ret = gpio_add_callback(gpio_device, &gpio_cb);
    if (ret)
    {
        LOG_ERR("Error enabling callback %d!\n", MAX14830_IRQ);
    }
    ret = gpio_pin_enable_callback(gpio_device, MAX14830_IRQ);
    if (ret)
    {
        LOG_ERR("Error enabling callback %d!\n", MAX14830_IRQ);
    }

    max14830_write(address, IRQENABLE_REGISTER, RFifoTrgIEn | LSRErrIEn);
    max14830_write(address, FIFOTRIGLVL_REGISTER, (1 << 4));
    max14830_write(address, MODE1_REGISTER, IRQSel);

    // Enable RTimeout interrupt
    max14830_write(address, LSRINTEN_REGISTER, 0b00001111);
}

static void MAX_RX_entry_point(void *foo, void *bar, void *gazonk)
{
    LOG_INF("MAX RX Thread running...\n");

    while (true)
    {
        k_sem_take(&rx_sem, K_FOREVER);
        LOG_INF("GOT a response!");
        // why this? Are there any things we should know about?
        k_sleep(1000);

        readReply();
    }
}


static max_char_callback_t char_callback = NULL;

void MAX_init(max_char_callback_t cb)
{
    LOG_INF("Initializing MAX14830...\n");
    resetWait();

    char_callback = cb;

    // Initialize baud rate, parity, word length and stop bits for each uart
    initUart(EE_NBIOT_01_ADDRESS, 9600, 8, NO_PARITY, 1);
    EnableRxFIFOIrq(EE_NBIOT_01_ADDRESS);
    max14830_read(EE_NBIOT_01_ADDRESS, INTERRUPT_STATUS_REGISTER); // What the actual .... ?

    k_sem_init(&rx_sem, 0, RX_SEM_SIZE);

    k_thread_create(&max_thread, max_thread_stack,
                    K_THREAD_STACK_SIZEOF(max_thread_stack),
                    (k_thread_entry_t)MAX_RX_entry_point,
                    NULL, NULL, NULL, MAX_THREAD_PRIORITY, 0, K_NO_WAIT);
}
