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
#include "txrxbuffer.h"
#include "max14830.h"
#include <string.h>
#include <stdbool.h>
#include <stdio.h>


/*
const char OPEN_SOCKET_CMD[] = {"AT+NSOCR=\"DGRAM\",17,%d,%d\r"};
const char CLOSE_SOCKET_CMD[] = {"AT+NSOCL=%d\r"};
const char SEND_MESSAGE_CMD[] = {"AT+NSOST=%d,\"172.16.15.14\",%d,%d,\"%s\"\r"};
const char CHECK_IP_ADDR_CMD[] = {"AT+CGPADDR\r"};
const char REBOOT_CMD[] = {"AT+NRB\r"};
const char ERROR_STATUS_CMD[] = {"AT+CMEE=1\r"};

extern uint8_t RX_BUFFER[RX_BUFFER_SIZE];
uint8_t TX_REWRITE_BUFFER[TX_BUFFER_SIZE];

int reset_ee_nbiot_01_pin = 02;



bool hasIPAddress() 
{
    //console_printf("Checking if the device has been asigned an IP address.\n");
    char * cmd = (char *)CHECK_IP_ADDR_CMD;
    char * response = (char*)RX_BUFFER;
    console_printf("%s", cmd);
    sendMessage(EE_NBIOT_01_ADDRESS, (uint8_t*)CHECK_IP_ADDR_CMD, RX_BUFFER);

    if (strstr(response, "ERROR\r"))
    {
        console_printf("Error executing AT command: %s. Response: %s\n", cmd, response);
        return false;
    }
    if (!strstr(response, "OK\r"))
    {
        console_printf("Incomplete response executing AT command: %s. Response: %s\n", cmd, response);
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
        console_printf("YAAAAY!, we've got an IP address!\n");
        return true;
    }

    console_printf("BUMMER!, No IP address yet...\n");
    return false;
}

int openSocket(int port, RECEIVE_CONTROL ctrl)
{
    console_printf("Opening socket.\n");

    sprintf((char*)TX_REWRITE_BUFFER, (char*)OPEN_SOCKET_CMD, port, ctrl);
    console_printf("%s\n", TX_REWRITE_BUFFER);
    sendMessage(EE_NBIOT_01_ADDRESS, TX_REWRITE_BUFFER, RX_BUFFER);
    char * cmd = (char*)TX_REWRITE_BUFFER;
    char * response = (char*)RX_BUFFER;
    console_printf("Open socket response:%s\n", response);

    if (strstr(response, "ERROR\r"))
    {
        console_printf("Error executing AT command: %s. Response: %s\n", cmd, response);
        return -1;
    }
    if (!strstr(response, "OK\r"))
    {
        console_printf("Incomplete response executing AT command: %s. Response: %s\n", cmd, response);
        return -1;
    }

    const char * separator = "\r";
    const char * token = strtok(response, separator);
    return atoi(token);
}

bool closeSocket(int socket)
{
    console_printf("Closing socket.\n");

    sprintf((char*)TX_REWRITE_BUFFER, (char*)CLOSE_SOCKET_CMD, socket);
    console_printf("%s\n", TX_REWRITE_BUFFER);
    sendMessage(EE_NBIOT_01_ADDRESS, TX_REWRITE_BUFFER, RX_BUFFER);
    char * cmd = (char*)TX_REWRITE_BUFFER;
    char * response = (char*)RX_BUFFER;
    console_printf("Close socket response:%s\n", response);

    if (strstr(response, "ERROR\r"))
    {
        console_printf("Error executing AT command: %s. Response: %s\n", cmd, response);
        return false;
    }
    if (!strstr(response, "OK\r"))
    {
        console_printf("Incomplete response executing AT command: %s. Response: %s\n", cmd, response);
        return true;
    }

    return false;
}

bool sendNBIoTMessage(int port, char * message)
{
    console_printf("Sending NB-IoT message.\n");

    bool retVal = false;
    int socket = openSocket(port, IGNORE_INCOMING_MESSAGES);

    sprintf((char*)TX_REWRITE_BUFFER, (char*)SEND_MESSAGE_CMD, socket, port, strlen(message)/2, message);
    //console_printf("REWRITTEN MESSAGE: %s\n", TX_REWRITE_BUFFER);
    sendMessage(EE_NBIOT_01_ADDRESS, TX_REWRITE_BUFFER, RX_BUFFER);

    char * cmd = (char*)TX_REWRITE_BUFFER;
    char * response = (char*)RX_BUFFER;
    console_printf("Send message response:%s\n", response);

    if (strstr(response, "ERROR\r"))
    {
        console_printf("Error executing AT command: %s. Response: %s\n", cmd, response);
    }
    if (!strstr(response, "OK\r"))
    {
        console_printf("Incomplete response executing AT command: %s. Response: %s\n", cmd, response);
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
    console_printf("Rebooting SARA module\n");
    hal_gpio_write(reset_ee_nbiot_01_pin, 0);
    os_time_delay(OS_TICKS_PER_SEC);
    hal_gpio_write(reset_ee_nbiot_01_pin, 1);
    os_time_delay(OS_TICKS_PER_SEC);

    os_time_delay(OS_TICKS_PER_SEC*4);

    sendMessage(EE_NBIOT_01_ADDRESS, (uint8_t*)ERROR_STATUS_CMD, RX_BUFFER);
    console_printf("Response: %s\n", RX_BUFFER);
}

void init_ee_nbiot_01()
{
    console_printf("NB-IoT init\n");

    hal_gpio_init_out(reset_ee_nbiot_01_pin, 1);

    while (!hasIPAddress()) 
    {
        os_time_delay(OS_TICKS_PER_SEC*2);
    }
}

*/