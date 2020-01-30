#ifndef _MESSAGE_BUFFER_H_
#define _MESSAGE_BUFFER_H_

#include "opc_n3.h"
#include "gps_cache.h"
#include "chipcap2.h"
#include "ads124s08.h"

typedef struct 
{
    gps_fix_t   gps_fix;
    CC2_SAMPLE  cc2_sample;
    OPC_SAMPLE  opc_sample;
    AFE3_SAMPLE afe3_sample;

} SENSOR_NODE_SAMPLE;

typedef union 
{
    SENSOR_NODE_SAMPLE  sample;
    uint8_t             bytebuffer[sizeof(SENSOR_NODE_SAMPLE)];
} SENSOR_NODE_MESSAGE;

void DEBUG_CC2();
void DEBUG_GPS();
void DEBUG_OPC();
void DEBUG_AFE();


#endif


