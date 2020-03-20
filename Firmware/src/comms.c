#include <zephyr.h>
#include <device.h>
#include <kernel.h>
#include <sys/ring_buffer.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include "max14830.h"
#include "comms.h"
#include "at_commands.h"
#include "init.h"
#include "priorities.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(COMMS, CONFIG_COMMS_LOG_LEVEL);

// Ring buffer for received data
#define RB_SIZE 255
static u8_t buffer[RB_SIZE];
static struct ring_buf rx_rb;
static struct k_sem urc_sem;

// Ring buffer for URCs. These are handled separately
#define URC_SIZE 64
static uint8_t urcbuffer[URC_SIZE];
static struct ring_buf urc_rb;
static struct k_sem rx_sem;

#define URC_THREAD_STACK 512
#define DUMP_MODEM 0

struct k_thread urc_thread;

K_THREAD_STACK_DEFINE(urc_thread_stack,
                      URC_THREAD_STACK);

static recv_callback_t recv_cb = NULL;

/**
 * Receive callback for the sockets. This calls receive_cb when
 * +NSOMNMI URC is received
 */
void receive_callback(recv_callback_t receive_cb)
{
    recv_cb = receive_cb;
}

/**
 * URC detecting procedure
 */
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
                        LOG_DBG("nsonmi");
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

/**
 * @brief Callback for chars from the modem
 */
void comms_handle_char(uint8_t data)
{
    static char prev = '\n';
    static bool in_urc = false;
    int rb;
#if DUMP_MODEM
    printf("%c", data);
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
    if (rb != 1)
    {
        LOG_ERR("RX buffer is full");
        return;
    }
    prev = data;
    k_sem_give(&rx_sem);
}

void modem_write(const char *cmd)
{
#if DUMP_MODEM
    printf("%s", cmd);
#endif
    max_send(cmd, strlen(cmd));
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
        LOG_DBG("Timed out with timeout set to %d", timeout);
        break;
    }
    return false;
}

bool modem_is_ready()
{
    modem_write("AT+CGPADDR\r");
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

void modem_configure()
{
    modem_write("AT+CGDCONT=0,\"IP\",\"mda.ee\"\r");
    if (at_generic_decode() != AT_OK)
    {
        LOG_ERR("AT+CGPADDR (APN config) did not return OK");
    }
    modem_write("AT+NCONFIG=\"AUTOCONNECT\",\"TRUE\"\r");
    if (at_generic_decode() != AT_OK)
    {
        LOG_ERR("AT+NCONFIG (auto connect) did not return OK");
    }
}

void modem_restart()
{
    modem_write("AT+NRB\r");
    atnrb_decode();
}

void modem_restart_without_triggering_network_signalling_storm_but_hopefully_picking_up_the_correct_cell___maybe()
{
    modem_write("AT+NRB\r");
    modem_write("AT+CFUN=0\r");
    atnrb_decode();
    modem_write("AT+NRB\r");
    modem_write("AT+CFUN=1\r");
    atnrb_decode();

    LOG_DBG("Waiting for modem to connect...");
    while (!modem_is_ready())
    {
        k_sleep(K_MSEC(2000));
    }
}

#define IMEI_IMSI_BUF_SIZE 24
char imsi[IMEI_IMSI_BUF_SIZE];
char imei[IMEI_IMSI_BUF_SIZE];
char imei_unparsed[IMEI_IMSI_BUF_SIZE];



void modem_init(void)
{
    LOG_INF("Init modem");
    k_sem_init(&rx_sem, 0, RB_SIZE);
    ring_buf_init(&rx_rb, RB_SIZE, buffer);
    k_sem_init(&urc_sem, 0, URC_SIZE);
    ring_buf_init(&urc_rb, URC_SIZE, urcbuffer);

    k_thread_create(&urc_thread, urc_thread_stack,
                    K_THREAD_STACK_SIZEOF(urc_thread_stack),
                    (k_thread_entry_t)urc_threadproc,
                    NULL, NULL, NULL, URC_THREAD_PRIORITY, 0, K_NO_WAIT);

    max_init(comms_handle_char);

    // OK. This is clutching at straws
    LOG_DBG("Modem startup");
    // Modem starts up rebooting so wait for the initial OK.
    atnrb_decode();
    LOG_DBG("Startup complete");

    LOG_DBG("Configure APN");
    modem_configure();

    LOG_DBG("Modem restart");
    modem_restart();

    LOG_DBG("Waiting for modem to connect...");
    while (!modem_is_ready())
    {
        k_sleep(K_MSEC(2000));
    }

    memset(imsi, 0, IMEI_IMSI_BUF_SIZE);
    modem_write("AT+CIMI\r");
    if (atcimi_decode((char *)&imsi) != AT_OK)
    {
        LOG_ERR("Unable to retrieve IMSI from modem");
    }
    else
    {
        LOG_INF("IMSI for modem is %s", log_strdup(imsi));
    }

    // Using same decoder as IMSI
    memset(imei, 0, IMEI_IMSI_BUF_SIZE);
    memset(imei_unparsed, 0, IMEI_IMSI_BUF_SIZE);
    
    modem_write("AT+CGSN=1\r");
    if (atcgsn_decode((char *)&imei_unparsed) != AT_OK) 
    {
        LOG_ERR("Unable to retrieve IMEI from modem");
    }
    else
    {
        strcpy(imei, &imei_unparsed[6]);
        LOG_INF("IMEI for modem is %s", log_strdup(imei));
    }
 }

