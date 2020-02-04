#include <zephyr.h>
#include <stdio.h>
#include <string.h>
#include "messagebuffer.h"
#include "gps_cache.h"

SENSOR_NODE_MESSAGE sensor_node_message;

#define TRANSMIT_BUFFER_SIZE sizeof(SENSOR_NODE_MESSAGE)+1

static uint8_t index = 0;
static uint8_t txbuffer[TRANSMIT_BUFFER_SIZE];

static void mb_reset() {
    index = 0;
    memset(txbuffer, 0, TRANSMIT_BUFFER_SIZE);
}

static void mb_append_uint32(uint32_t value)
{
    txbuffer[index++] = (uint8_t)((value & 0xff000000) >> 24);
    txbuffer[index++] = (uint8_t)((value & 0x00ff0000) >> 16);
    txbuffer[index++] = (uint8_t)((value & 0x0000ff00) >> 8);
    txbuffer[index++] = (uint8_t)(value & 0x000000ff);
}

static void mb_append_uint16(uint16_t value)
{
    txbuffer[index++] = (uint8_t)((value & 0xff00) >> 8);
    txbuffer[index++] = (uint8_t)(value & 0x00ff);
}

static void mb_append_uint8(uint8_t value)
{
    txbuffer[index++] = value;
}

uint8_t *mb_encode()
{
    mb_reset();
    // GPS
    // Index    Description
    // 0 - 8    GPS timestamp
    // 9 - 16   GPS longitude
    // 17 - 24  GPS latitude
    // 25 - 32  GPS altitude
    mb_append_uint32(sensor_node_message.gps_fix.timestamp);
    mb_append_uint32(sensor_node_message.gps_fix.longitude);
    mb_append_uint32(sensor_node_message.gps_fix.latitude);
    mb_append_uint32(sensor_node_message.gps_fix.altitude);

    // Board stats
    // Index    Description
    // 33 - 40  Board temperature
    // 41 - 48  Board humidity
    mb_append_uint32(sensor_node_message.cc2_sample.Temp_C);
    mb_append_uint32(sensor_node_message.cc2_sample.RH);

    // AFE ADC
    // Index    Description
    // 49 - 56  OP1 ADC reading - Nitrogen Dioxide working electrode
    // 57 - 64  OP2 ADC reading - Nitrogen Dioxide auxillary electrode
    // 65 - 72  OP3 ADC reading - Ozone + Nitrogen Dioxide working electrode
    // 73 - 80  OP4 ADC reading - Ozone + Nitrogen Dioxide auxillary electrode
    // 81 - 88  OP5 ADC reading - Nitric Oxide Working electrode
    // 89 - 96  OP6 ADC reading - Nitric Oxide auxillary electrode
    // 97 - 104  Pt1000 ADC reading - AFE-3 ambient temperature
    mb_append_uint32(sensor_node_message.afe3_sample.op1);
    mb_append_uint32(sensor_node_message.afe3_sample.op2);
    mb_append_uint32(sensor_node_message.afe3_sample.op3);
    mb_append_uint32(sensor_node_message.afe3_sample.op4);
    mb_append_uint32(sensor_node_message.afe3_sample.op5);
    mb_append_uint32(sensor_node_message.afe3_sample.op6);
    mb_append_uint32(sensor_node_message.afe3_sample.pt);

    // OPC-N3
    // Index    Description
    // 105 - 112  OPC PM A (default PM1)
    // 113 - 120  OPC PM B (default PM2.5)
    // 121 - 128  OPC PM C (default PM10)
    // 129 - 132  OPC sample period
    // 133 - 136  OPC sample flowrate
    // 137 - 140  OPC temperature
    // 141 - 144  OPC humidity
    // 145 - 148  OPC fan rev count
    // 149 - 152  OPC laser status
    // 153 - 156  Histogram count - bin 1
    // 157 - 160  Histogram count - bin 2
    // ...
    // 241 - 244  Histogram count - bin 24
    // 245 - 246  Valid / invalid data (bool)
    mb_append_uint32(sensor_node_message.opc_sample.pm_a);
    mb_append_uint32(sensor_node_message.opc_sample.pm_b);
    mb_append_uint32(sensor_node_message.opc_sample.pm_c);
    mb_append_uint16(sensor_node_message.opc_sample.period);
    mb_append_uint16(sensor_node_message.opc_sample.flowrate);
    mb_append_uint16(sensor_node_message.opc_sample.temperature);
    mb_append_uint16(sensor_node_message.opc_sample.fan_rev_count);
    mb_append_uint16(sensor_node_message.opc_sample.laser_status);

    for (int i = 0; i < OPC_BINS; i++)
    {
        mb_append_uint16(sensor_node_message.opc_sample.bin[i]);
    }
    mb_append_uint8(sensor_node_message.opc_sample.valid);

    return txbuffer;
}

void DEBUG_CC2()
{
    printf("Chipcap 2 / board status:\n");
    printf("   > Main board temperature: %.2f (C)\n", sensor_node_message.cc2_sample.Temp_C);
    printf("   > Main board humidity: %.2f (%%)\n", sensor_node_message.cc2_sample.RH);
}

void DEBUG_GPS()
{
    printf("GPS - status:\n");

    printf("   > Timestamp: %f\n", sensor_node_message.gps_fix.timestamp);
    printf("   > Altitude: %f \n", sensor_node_message.gps_fix.altitude);
    printf("   > Longitude: %f (rad)\n", sensor_node_message.gps_fix.longitude);
    printf("   > Latitude: %f (rad)\n", sensor_node_message.gps_fix.latitude);
}

void DEBUG_OPC()
{
    printf("OPC-N3 - status:\n");
    printf("   > PM A: %f\n", sensor_node_message.opc_sample.pm_a);
    printf("   > PM B: %f\n", sensor_node_message.opc_sample.pm_b);
    printf("   > PM C: %f\n", sensor_node_message.opc_sample.pm_c);
    printf("    > Period: %d\n", sensor_node_message.opc_sample.period);
    printf("    > Temperature: %d\n", sensor_node_message.opc_sample.temperature);
    printf("    > Humidity: %d\n", sensor_node_message.opc_sample.humidity);
    printf("    > Validity: %s\n", sensor_node_message.opc_sample.valid ? "YES" : "NO");
    printf("    > Histogram:\n");
    for (int i = 0; i < OPC_BINS; i++)
    {
        printf("        bin%02d : %d\n", i, sensor_node_message.opc_sample.bin[i]);
    }
}

void DEBUG_AFE()
{
    printf("AFE - status:\n");
    printf("    > OP1: %u\n", sensor_node_message.afe3_sample.op1);
    printf("    > OP2: %u\n", sensor_node_message.afe3_sample.op2);
    printf("    > OP3: %u\n", sensor_node_message.afe3_sample.op3);
    printf("    > OP4: %u\n", sensor_node_message.afe3_sample.op4);
    printf("    > OP5: %u\n", sensor_node_message.afe3_sample.op5);
    printf("    > OP6: %u\n", sensor_node_message.afe3_sample.op6);
    printf("    > PT: %u\n", sensor_node_message.afe3_sample.pt);
}
