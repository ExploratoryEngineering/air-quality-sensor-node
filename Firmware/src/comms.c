#include "config.h"

#define LOG_LEVEL LOG_LEVEL_INF
#include <logging/log.h>
LOG_MODULE_REGISTER(n2_comms);


#include <zephyr.h>
#include <device.h>
#include <uart.h>
#include <kernel.h>
#include <sys/ring_buffer.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include "comms.h"
#include "at_commands.h"

// Ring buffer for received data
#define RB_SIZE 128
static u8_t buffer[RB_SIZE];
static struct ring_buf rx_rb;
static struct k_sem urc_sem;

// Ring buffer for URCs. These are handled separately
#define URC_SIZE 64
static uint8_t urcbuffer[URC_SIZE];
static struct ring_buf urc_rb;
static struct k_sem rx_sem;


#define URC_THREAD_STACK 512
#define URC_THREAD_PRIORITY (CONFIG_NUM_COOP_PRIORITIES)
#define UART_NAME "UART_0"
#define DUMP_MODEM 0

struct k_thread urc_thread;

K_THREAD_STACK_DEFINE(urc_thread_stack,
                      URC_THREAD_STACK);

static recv_callback_t recv_cb = NULL;

void receive_callback(recv_callback_t receive_cb)
{
    recv_cb = receive_cb;
}

void urc_threadproc(void)
{
    char buf[URC_SIZE];
    uint8_t index = 0;
    uint8_t b = 0;
    while (true)
    {
        k_sem_take(&urc_sem, K_FOREVER);
        if (ring_buf_get(&urc_rb, &b, 1) == 1)
        {
            if (b == '\r')
            {
                // this is a new URC
                buf[index] = 0;
                if (index > 0)
                {
                    if (recv_cb && strncmp(buf, "+NSONMI:", 8) == 0)
                    {
                        // This is a receive notification. Invoke callback
                        char *countptr = NULL;
                        char *fdptr = buf;
                        for (uint8_t i = 0; i < index; i++)
                        {
                            if (buf[i] == ',')
                            {
                                countptr = (buf + i + 1);
                                buf[i] = 0;
                            }
                        }
                        // Note offset -- this is the "+NSONMI: " prefix
                        recv_cb(atoi(fdptr + 9), atoi(countptr));
                    }
                }
                index = 0;
            }
            if (b != '\r' && b != '\n')
            {
                buf[index++] = b;
            }
        }
    }
}
/*
 * Modem comms. It's quite a mechanism - the UART is read from an ISR, then
 * sent to a processing thread via a ring buffer. The processing thread
 * parses the incoming data stream and when OK or ERROR is received the
 * data is forwarded to the consuming library via modem_read_line.
 */

/**
 * @brief The ISR for UART rx
 */
static void uart_isr(void *user_data)
{
    static char prev = '\n';
    static bool in_urc = false;
    struct device *dev = (struct device *)user_data;
    uint8_t data;
    int rx, rb;
    while (uart_irq_update(dev) &&
           uart_irq_rx_ready(dev))
    {
        rx = uart_fifo_read(dev, &data, 1);
        if (rx == 0)
        {
            return;
        }
#if DUMP_MODEM
        printk("%c", data);
#endif
        if (prev == '\n' && data == '+')
        {
            in_urc = true;
        }
        if (in_urc)
        {
            ring_buf_put(&urc_rb, &data, 1);
            k_sem_give(&urc_sem);
        }
        if (in_urc && data == '\r')
        {
            in_urc = false;
        }
        rb = ring_buf_put(&rx_rb, &data, 1);
        if (rb != rx)
        {
            LOG_ERR("RX buffer is full. Bytes pending: %d, written: %d", rx, rb);
            return;
        }
        prev = data;
        k_sem_give(&rx_sem);
    }
}

void modem_write(const char *cmd)
{
#if DUMP_MODEM
    printk("%s", cmd);
#endif
    struct device *uart_dev = device_get_binding(UART_NAME);
	if (!uart_dev) {
		LOG_ERR("Cannot get UART device");
		return;
	}

    for (int i = 0; i < strlen(cmd); i++)
    {
        uart_poll_out(uart_dev, (unsigned char)cmd[i]);
    }
}

bool modem_read(uint8_t *b, int32_t timeout)
{
    switch (k_sem_take(&rx_sem, timeout))
    {
    case 0:
        if (ring_buf_get(&rx_rb, b, 1) == 1)
        {
            return true;
        }
        break;
    default:
        break;
    }
    return false;
}

bool modem_is_ready()
{
    modem_write("AT+CGPADDR\r\n");
    char ip[16];
    size_t len = 0;
    if (atcgpaddr_decode((char *)&ip, &len) == AT_OK)
    {
        if (len > 1)
        {
            return true;
        }
    }
    return false;
}

void modem_restart()
{
    modem_write("AT+NRB\r\n");
    atnrb_decode();
}

void modem_init(void)
{
    k_sem_init(&rx_sem, 0, RB_SIZE);
    ring_buf_init(&rx_rb, RB_SIZE, buffer);
    k_sem_init(&urc_sem, 0, URC_SIZE);
    ring_buf_init(&urc_rb, URC_SIZE, urcbuffer);

    k_thread_create(&urc_thread, urc_thread_stack,
                    K_THREAD_STACK_SIZEOF(urc_thread_stack),
                    (k_thread_entry_t)urc_threadproc,
                    NULL, NULL, NULL, K_PRIO_COOP(URC_THREAD_PRIORITY), 0, K_NO_WAIT);


    struct device *uart_dev = device_get_binding(UART_NAME);
    if (!uart_dev)
    {
        LOG_ERR("Unable to load UART device\n");
        return;
    }

    uart_irq_callback_user_data_set(uart_dev, uart_isr, uart_dev);
    uart_irq_rx_enable(uart_dev);

    // Set up the modem. Might also include AT+CGPADDR to set up PDP context
    // here.
    modem_restart();

    LOG_INF("Waiting for modem to connect...");
    while (!modem_is_ready())
    {
        k_sleep(K_MSEC(2000));
    }
    modem_write("AT+CIMI\r");
    char imsi[24];
    if (atcimi_decode((char *)&imsi) != AT_OK)
    {
        LOG_ERR("Unable to retrieve IMSI from modem");
    }
    else
    {
        LOG_INF("IMSI for modem is %s", log_strdup(imsi));
    }
}
