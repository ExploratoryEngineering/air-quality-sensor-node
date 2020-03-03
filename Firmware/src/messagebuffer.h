#pragma once

#include <zephyr.h>
#include "opc_n3.h"
#include "gps_cache.h"
#include "chipcap2.h"
#include "ads124s08.h"

typedef struct
{
    gps_fix_t gps_fix;
    CC2_SAMPLE cc2_sample;
    OPC_SAMPLE opc_sample;
    AFE3_SAMPLE afe3_sample;
    s64_t uptime;

} SENSOR_NODE_MESSAGE;

/**
 * @brief Encode message
 */
size_t mb_encode(SENSOR_NODE_MESSAGE *msg, char *buf, size_t max);

/**
 * @brief Dump the message (debug)
 */
void mb_dump_message(SENSOR_NODE_MESSAGE *msg);

/**
 * @brief Hex dump of the messagebuffer (debug)
 */
void mb_hex_dump_message(char *buf, size_t len);
