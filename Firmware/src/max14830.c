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
#include "priorities.h"

#define LOG_LEVEL CONFIG_MAX14830_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(MAX14830);

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

#define MAX_THREAD_STACK_SIZE 1024

//#define DUMP_OUTPUT 1

struct k_thread max_thread;

K_THREAD_STACK_DEFINE(max_thread_stack, MAX_THREAD_STACK_SIZE);

static struct k_sem rx_sem;
#define RX_SEM_SIZE 255

static max_char_callback_t char_callback;

static struct gpio_callback gpio_cb;

static int max_write(uint8_t address, uint8_t reg, uint8_t data)
{
    uint8_t txBuffer[] = {reg, data};
    int err = i2c_write(get_I2C_device(), txBuffer, 2, address);
    if (0 != err)
    {
        LOG_ERR("i2c_write failed with error: %d", err);
    }
    return err;
}

static uint8_t max_read(uint8_t address, uint8_t reg)
{
    uint8_t txBuffer[] = {reg};
    uint8_t rxBuffer[] = {0};

    int err = i2c_write_read(get_I2C_device(), address, txBuffer, 1, rxBuffer, 1);
    if (0 != err)
    {
        LOG_ERR("i2c_write_read failed with error: %d", err);
    }

    return rxBuffer[0];
}

static void init_uart(uint8_t address, int baud, uint8_t wordlength, uint8_t parity, uint8_t stopBits)
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
    LOG_DBG("MAX14830: Base clock frequency: %d, Address: %02X, UART baudrate: %d, divisor: %d, mode: %d", CLOCK_FREQUENCY, address, baud, div, mode);

    max_write(address, DIVLSB_REGISTER, div / 16);
    max_write(address, DIVMSB_REGISTER, (div / 16) >> 8);
    max_write(address, BRGCONFIG_REGISTER, (div % 16) | mode);

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

    max_write(address, LCR_REGISTER, lcr);
    max_write(address, CLKSOURCE_REGISTER, 0b00001010); // PLLBypass

    // Set up rxTimeout
    max_write(address, RXTIMEOUT_REGISTER, 0b00000100);
}

static void wait_for_reset()
{
    LOG_DBG("MAX14830: Waiting for reset signal...");
    u32_t resetVal;
    do
    {
        LOG_DBG("MAX14830: Waiting...");
        gpio_pin_read(get_GPIO_device(), MAX14830_IRQ, &resetVal);
    } while (0 == resetVal);
}

static void enable_rx_mode(uint8_t address)
{
    uint8_t mode1 = max_read(address, MODE1_REGISTER);
    mode1 |= 0b00000010;
    max_write(address, MODE1_REGISTER, mode1);
}

static void enable_tx_mode(uint8_t address)
{
    uint8_t mode1 = max_read(address, MODE1_REGISTER);
    mode1 &= 0b10000001;
    max_write(address, MODE1_REGISTER, mode1);
}

static void wait_for_tx(uint8_t address)
{
    while (max_read(address, TxFIFOLvl_REGISTER))
    {
        k_sleep(1);
    }
}

static void read_from_rx_fifo(uint8_t address)
{
    uint8_t fifo_level = 0;
    uint8_t ch;
    while (true)
    {
        fifo_level = max_read(address, RxFIFOLvl_REGISTER);
        if (0 == fifo_level)
        {
            max_read(address, INTERRUPT_STATUS_REGISTER);
            return;
        }
        ch = max_read(address, RHR_REGISTER);
        if (char_callback)
        {
#ifdef DUMP_OUTPUT
            printf("%c", ch);
#endif
            char_callback(ch);
        }
    }
}

static void read_reply()
{
    // The trigger can be read from any UART
    uint8_t uart_trigger = max_read(EE_NBIOT_01_ADDRESS, GLOBAL_IRQ_REGISTER);

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
    uint8_t cause = max_read(uart_address, INTERRUPT_STATUS_REGISTER);
    if (cause & LSRErrInt)
    {
        // Check the line status register
        uint8_t line_status = max_read(uart_address, LSR_REGISTER);
        if ((line_status & RTimeout) || (line_status & RFifoTrigInt))
        {
            read_from_rx_fifo(uart_address);
        }
    }
}

static void irq_handler(struct device *gpiob, struct gpio_callback *cb, u32_t pins)
{
    k_sem_give(&rx_sem);
}

static void enable_rx_fifo_irq(uint8_t address)
{
    struct device *gpio_device = get_GPIO_device();
    int ret = gpio_pin_configure(gpio_device, MAX14830_IRQ, GPIO_INT | GPIO_PUD_PULL_UP | GPIO_INT_EDGE | GPIO_INT_ACTIVE_LOW | GPIO_DIR_IN);
    if (ret)
    {
        LOG_ERR("Error configuring %d!", MAX14830_IRQ);
    }
    gpio_init_callback(&gpio_cb, irq_handler, BIT(MAX14830_IRQ));
    ret = gpio_add_callback(gpio_device, &gpio_cb);
    if (ret)
    {
        LOG_ERR("Error enabling callback %d!", MAX14830_IRQ);
    }
    ret = gpio_pin_enable_callback(gpio_device, MAX14830_IRQ);
    if (ret)
    {
        LOG_ERR("Error enabling callback %d!", MAX14830_IRQ);
    }

    max_write(address, IRQENABLE_REGISTER, RFifoEmptyInt | RFifoTrgIEn | LSRErrIEn);
    max_write(address, FIFOTRIGLVL_REGISTER, 0b00010000);
    max_write(address, MODE1_REGISTER, IRQSel);

    // Enable RTimeout interrupt
    max_write(address, LSRINTEN_REGISTER, 0b01001111);
}

static void max_proc(void *param)
{
    LOG_DBG("MAX RX Thread running");

    while (true)
    {
        k_sem_take(&rx_sem, K_FOREVER);
        read_reply();
    }
}

int max_send(const uint8_t *buf, uint8_t len)
{
    enable_tx_mode(EE_NBIOT_01_ADDRESS);

    for (int i = 0; i < len; i++)
    {
        max_write(EE_NBIOT_01_ADDRESS, THR_REGISTER, *buf++);
    }

    wait_for_tx(EE_NBIOT_01_ADDRESS);
    enable_rx_mode(EE_NBIOT_01_ADDRESS);

    return 0;
}

static max_char_callback_t char_callback = NULL;

void max_init(max_char_callback_t cb)
{
    LOG_INF("Initializing");
    wait_for_reset();

    char_callback = cb;

    // Initialize baud rate, parity, word length and stop bits for each uart
    init_uart(EE_NBIOT_01_ADDRESS, 9600, 8, NO_PARITY, 1);
    enable_rx_fifo_irq(EE_NBIOT_01_ADDRESS);
    max_read(EE_NBIOT_01_ADDRESS, INTERRUPT_STATUS_REGISTER); // What the actual .... ?

    k_sem_init(&rx_sem, 0, RX_SEM_SIZE);

    k_thread_create(&max_thread, max_thread_stack,
                    K_THREAD_STACK_SIZEOF(max_thread_stack),
                    (k_thread_entry_t)max_proc,
                    NULL, NULL, NULL, MAX_THREAD_PRIORITY, 0, K_NO_WAIT);
}
