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

#pragma once

#include <stdint.h>

#define OPC_HISTOGRAM_SIZE 86

// Data offsets in the histogram
#define OPC_BINS 24
#define OPC_BIN_0_INDEX 0               // 16 bit unsigned integers
#define OPC_SAMPLING_PERIOD_INDEX 52    /* Sampling period is a 16 bit unsigned \
                                            integer and                         \
                                           is a measure of the histogram's      \
                                           actual sampling period in seconds x 100 */
#define OPC_SAMPLE_FLOWRATE_INDEX 54    /* Sampling period is a 16 bit unsigned integer and   \
                                           is a measure of the sample flow rate in ml/s x 100 \
                                        */
#define OPC_TEMPERATURE_INDEX 56        // 16 bit unsigned integer
#define OPC_HUMIDITY_INDEX 58           // 16 bit unsigned integer
#define OPC_PM_A_INDEX 60               // float. Units are ug/m3
#define OPC_PM_B_INDEX 64               // float. Units are ug/m3
#define OPC_PM_C_INDEX 68               // float. Units are ug/m3
#define OPC_FAN_REV_COUNT_INDEX 80      // 16 bit unsigned integer
#define OPC_LASER_STATUS_INDEX 82       // 16 bit unsigned integer
#define OPC_HISTOGRAM_CHECKSUM_INDEX 84 // 16 bit unsigned integer

typedef struct
{
    uint16_t bin[OPC_BINS];
    uint16_t period;
    uint16_t flowrate;
    uint16_t temperature;
    uint16_t humidity;
    float pm_a;
    float pm_b;
    float pm_c;
    uint16_t fan_rev_count;
    uint16_t laser_status;
    bool valid;
} OPC_SAMPLE;

// OPC commands (not the complete set)
#define OPC_N3_WRITE_PERIPHERAL_POWER_STATUS 0x03
#define OPC_N3_READ_INFORMATION_STRING 0x3F
#define OPC_N3_READ_SERIAL_NUMBER_STRING 0x10
#define OPC_N3_READ_HISTOGRAM_DATA_AND_RESET_HISTOGRAM 0x30
#define OPC_N3_READ_PM_DATA_AND_RESET_HISTOGRAM 0x32
#define OPC_N3_READ_DAC_AND_POWER_STATUS 0x13
#define OPC_RESET 0x06

// OPC status codes
#define OPC_BUSY 0x31
#define OPC_N3_DATA_READY 0xF3

// OPC peripherals / options
#define OPC_OPTION_FAN_ON 0x03
#define OPC_OPTION_FAN_OFF 0x02
#define OPC_OPTION_LASER_ON 0x07
#define OPC_OPTION_LASER_OFF 0x06
#define OPC_OPTION_NONE 0xFF

// All settling times are in microseconds
#define FAN_SETTLING_TIME_MS 10000
#define LASER_SETTLING_TIME_MS 50
#define INITIATE_TRANSMISSION_SETTLING_TIME_US 10
#define BYTE_READ_SETTLING_TIME_US 10

// According to Alphasense User Manual OPC-N3 Optical
// Particle Counter Issue 2. Chapter 8 ("repeat interval ms")
#define OPC_SAMPLING_TIME_MS 5000

#define OPC_N3_CMD_COMMAND_ACK_WAIT 10
#define OPC_N3_SPI_BUFFER_RESET_WAIT 2000

typedef enum
{
    OPC_OK = 0,
    OPC_NOT_IMPLEMENTED = -1,
    OPC_UNEXPECTED_OPC_RESPONSE = -2,
    OPC_CRC_ERROR = -3
} OPC_N3_RESULT;

void opc_init();
void opc_n3_get_sample(OPC_SAMPLE *msg);
void opc_n3_sample_data();