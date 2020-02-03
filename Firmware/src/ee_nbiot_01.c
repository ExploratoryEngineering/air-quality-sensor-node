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

#include <zephyr.h>
#include "ee_nbiot_01.h"
#include "max14830.h"
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include <logging/log.h>
#include <stdlib.h>
#include <string.h>


#define LOG_LEVEL CONFIG_EE06_LOG_LEVEL
LOG_MODULE_DECLARE(EE06);

const char OPEN_SOCKET_CMD[] = {"AT+NSOCR=\"DGRAM\",17,%d,%d\r"};
const char CLOSE_SOCKET_CMD[] = {"AT+NSOCL=%d\r"};
const char SEND_MESSAGE_CMD[] = {"AT+NSOST=%d,\"172.16.15.14\",%d,%d,\"%s\"\r"};
const char CHECK_IP_ADDR_CMD[] = {"AT+CGPADDR\r"};
const char REBOOT_CMD[] = {"AT+NRB\r"};
const char ERROR_STATUS_CMD[] = {"AT+CMEE=1\r"};

#define TX_BUFFER_SIZE RX_BUFFER_SIZE
extern uint8_t RX_BUFFER[RX_BUFFER_SIZE];
uint8_t TX_REWRITE_BUFFER[TX_BUFFER_SIZE];

// int reset_ee_nbiot_01_pin = 02;

bool hasIPAddress() 
{
    LOG_INF("Checking if the device has been asigned an IP address.");
    char * cmd = (char *)CHECK_IP_ADDR_CMD;
    char * response = (char*)RX_BUFFER;
    LOG_INF("COMMAND: %s", log_strdup(cmd));
    sendMessage(EE_NBIOT_01_ADDRESS, (uint8_t*)CHECK_IP_ADDR_CMD, strlen(CHECK_IP_ADDR_CMD));

    LOG_INF("\nResponse length: >>>%d<<<\n",strlen(response));
    k_sleep(100);
    LOG_INF("\nResponse: >>>%s<<<\n",log_strdup(response));
    k_sleep(100);


    if (strstr(response, "ERROR\r"))
    {
        LOG_ERR("Error executing AT command: %s. Response: %s", log_strdup(cmd), log_strdup(response));
        return false;
    }
    if (!strstr(response, "OK\r"))
    {
        LOG_ERR("Incomplete response executing AT command: %s. Response: %s", log_strdup(cmd), log_strdup(response));
        return false;
    }


    const char * responseSeparator = "\"";
    char * responseToken = strtok(response, responseSeparator);
    responseToken = strtok(NULL, responseSeparator);

    const char * quadSeparator = ".";
    const char * quadToken = strtok(responseToken, quadSeparator);
    int validQuadCount = 0;
    while (quadToken != NULL) {
        int num = atoi(quadToken);
        // Slightly naive, but...
        if (((num == 0) && (0 == strcmp(quadToken, "0"))) || ((num > 0) && (num <256)))
        {
            validQuadCount++;
        }
        quadToken = strtok(NULL, quadSeparator);
    }
    if (validQuadCount == 4) {
        LOG_INF("YAAAAY!, we've got an IP address!");
        return true;
    }

    LOG_INF("BUMMER!, No IP address yet...");
    return false;
}

int openSocket(int port, RECEIVE_CONTROL ctrl)
{
    LOG_INF("Opening socket.");

    sprintf((char*)TX_REWRITE_BUFFER, (char*)OPEN_SOCKET_CMD, port, ctrl);
    LOG_INF("%s\n", log_strdup(TX_REWRITE_BUFFER));
    sendMessage(EE_NBIOT_01_ADDRESS, TX_REWRITE_BUFFER, strlen(TX_REWRITE_BUFFER));
    char * cmd = (char*)TX_REWRITE_BUFFER;
    char * response = (char*)RX_BUFFER;
    LOG_INF("Open socket response:%s", log_strdup(response));

    if (strstr(response, "ERROR\r"))
    {
        LOG_ERR("Error executing AT command: %s. Response: %s", log_strdup(cmd), log_strdup(response));
        return -1;
    }
    if (!strstr(response, "OK\r"))
    {
        LOG_ERR("Incomplete response executing AT command: %s. Response: %s", log_strdup(cmd), log_strdup(response));
        return -1;
    }

    const char * separator = "\r";
    const char * token = strtok(response, separator);
    return atoi(token);
}

bool closeSocket(int socket)
{
    LOG_INF("Closing socket.");

    sprintf((char*)TX_REWRITE_BUFFER, (char*)CLOSE_SOCKET_CMD, socket);
    LOG_INF("%s\n", log_strdup(TX_REWRITE_BUFFER));
    sendMessage(EE_NBIOT_01_ADDRESS, TX_REWRITE_BUFFER, strlen(TX_REWRITE_BUFFER));
    char * cmd = (char*)TX_REWRITE_BUFFER;
    char * response = (char*)RX_BUFFER;
    LOG_INF("Close socket response:%s", log_strdup(response));

    if (strstr(response, "ERROR\r"))
    {
        LOG_ERR("Error executing AT command: %s. Response: %s", log_strdup(cmd), log_strdup(response));
        return false;
    }
    if (!strstr(response, "OK\r"))
    {
        LOG_ERR("Incomplete response executing AT command: %s. Response: %s", log_strdup(cmd), log_strdup(response));
        return true;
    }

    return false;
}

bool sendNBIoTMessage(int port, char * message)
{
    LOG_INF("Sending NB-IoT message.");

    bool retVal = false;
    int socket = openSocket(port, IGNORE_INCOMING_MESSAGES);

    sprintf((char*)TX_REWRITE_BUFFER, (char*)SEND_MESSAGE_CMD, socket, port, strlen(message)/2, message);
    //console_printf("REWRITTEN MESSAGE: %s\n", TX_REWRITE_BUFFER);
    sendMessage(EE_NBIOT_01_ADDRESS, TX_REWRITE_BUFFER, strlen(TX_REWRITE_BUFFER));

    char * cmd = (char*)TX_REWRITE_BUFFER;
    char * response = (char*)RX_BUFFER;
    LOG_INF("Send message response:%s", log_strdup(response));

    if (strstr(response, "ERROR\r"))
    {
        LOG_ERR("Error executing AT command: %s. Response: %s\n", log_strdup(cmd), log_strdup(response));
    }
    if (!strstr(response, "OK\r"))
    {
        LOG_ERR("Incomplete response executing AT command: %s. Response: %s", log_strdup(cmd), log_strdup(response));
        retVal = true;
    }

    closeSocket(socket);
    return retVal;
}

// -------------------------------------------
// IMPORTANT: Don't reboot using the reset pin
//  Use AT+NRB instead 
//  (otherwise, the network gets confused)
// -------------------------------------------
void reboot_ee_nb_iot()
{
    LOG_INF("Rebooting SARA module - Work in progress...\n");
    // hal_gpio_write(reset_ee_nbiot_01_pin, 0);
    // os_time_delay(OS_TICKS_PER_SEC);
    // hal_gpio_write(reset_ee_nbiot_01_pin, 1);
    // os_time_delay(OS_TICKS_PER_SEC);

    // os_time_delay(OS_TICKS_PER_SEC*4);

    // sendMessage(EE_NBIOT_01_ADDRESS, (uint8_t*)ERROR_STATUS_CMD, RX_BUFFER);

    sendMessage(EE_NBIOT_01_ADDRESS, (uint8_t*)REBOOT_CMD, strlen(REBOOT_CMD));
    
    LOG_INF("Response: %s", RX_BUFFER);
}


void init_ee_nbiot_01()
{
    LOG_INF("NB-IoT init");

    while (!hasIPAddress()) 
    {
        LOG_INF("Waiting for IP address...");
        k_sleep(2000);
    }
}
