#include "fota_uart_interface.h"

#include <zephyr.h>
#include <i2c.h>
#include <logging/log.h>
#include "i2c_config.h"
#include "max14830.h"

#define LOG_LEVEL CONFIG_EE06_LOG_LEVEL
LOG_MODULE_DECLARE(EE06);

K_SEM_DEFINE(uart_semaphore, 1, 1);

/**
 * @brief writes one byte to the max14830 TX FIFO via the transmit hold register on UART0.
 *
 * @param data byte to be written to the TX FIFO.
 *
 * @retval true if successful.
 * @retval false if unsuccessful.
 */
bool uart_write(uint8_t data)
{
    if (0 != k_sem_take(&uart_semaphore, K_MSEC(100)))
    {
        LOG_ERR("Uart not available");
        return false;
    }

    uint8_t txBuffer[2];
    txBuffer[0] = THR_REGISTER;
    txBuffer[1] = data;
    bool ret = true;

    int err = i2c_write(get_I2C_device(), txBuffer, 2, EE_NBIOT_01_ADDRESS);
    if (0 != err)
    {
        LOG_ERR("(uart_write) i2c_write failed with error: %d", err);
        ret = false;
    }

    k_sem_give(&uart_semaphore);
    return ret;
}

/**
 * @brief polls the max14830 RX FIFO via the receive hold register on UART0.
 *
 * @param data byte to be read from the RX FIFO.
 *
 * @retval true if a byte could be read (the read byte is written to *data)
 * @retval false if unsuccessful or FIFO is empty.
 */
bool uart_poll(uint8_t * data)
{
    if (0 != k_sem_take(&uart_semaphore, K_MSEC(100)))
    {
        LOG_ERR("Uart not available");
        return false;
    }

    // Note: Subtly ignoring the UART trigger filtering here, since we're only using UART0. 
    // No way that this is going to come back and bite us at a later date...

    uint8_t txBuffer[] = {RxFIFOLvl_REGISTER};
    uint8_t rxBuffer[] = {0};
    int err = 0;
    bool ret = true;

    // Is there anything waiting in the RX-FIFO ?
    err = i2c_write_read(get_I2C_device(), EE_NBIOT_01_ADDRESS, txBuffer, 1, rxBuffer, 1);
    if (0 != err)
    {
        LOG_ERR("(uart_poll)i2c_write_read failed with error: %d", err);
        ret = false;
    } 
    else 
    {
        if (0 != rxBuffer[0])
        {
            // Read one byte from the receive hold register
            txBuffer[0] = RHR_REGISTER;
            err = i2c_write_read(get_I2C_device(), EE_NBIOT_01_ADDRESS, txBuffer, 1, rxBuffer, 1);
            if (0 != err)
            {
                LOG_ERR("(uart_poll)i2c_write_read failed with error: %d", err);
                ret = false;
            }
            *data = rxBuffer[0];
        }
    }

    k_sem_give(&uart_semaphore);
    return ret;
}
