#include <zephyr.h>
#include <stdio.h>
#include <string.h>
#include "messagebuffer.h"
#include "gps_cache.h"
#include <math.h>

static void mb_append_s64_t(uint8_t *buf, size_t *index, s64_t value)
{
    buf[(*index)++] = (uint8_t)((value & 0xff00000000000000) >> 56);
    buf[(*index)++] = (uint8_t)((value & 0x00ff000000000000) >> 48);
    buf[(*index)++] = (uint8_t)((value & 0x0000ff0000000000) >> 40);
    buf[(*index)++] = (uint8_t)((value & 0x000000ff00000000) >> 32);
    buf[(*index)++] = (uint8_t)((value & 0x00000000ff000000) >> 24);
    buf[(*index)++] = (uint8_t)((value & 0x0000000000ff0000) >> 16);
    buf[(*index)++] = (uint8_t)((value & 0x000000000000ff00) >> 8);
    buf[(*index)++] = (uint8_t)(value & 0x00000000000000ff);
}

static void mb_append_uint32(uint8_t *buf, size_t *index, uint32_t value)
{
    buf[(*index)++] = (uint8_t)((value & 0xff000000) >> 24);
    buf[(*index)++] = (uint8_t)((value & 0x00ff0000) >> 16);
    buf[(*index)++] = (uint8_t)((value & 0x0000ff00) >> 8);
    buf[(*index)++] = (uint8_t)(value & 0x000000ff);
}

static void mb_append_uint16(uint8_t *buf, size_t *index, uint16_t value)
{
    buf[(*index)++] = (uint8_t)((value & 0xff00) >> 8);
    buf[(*index)++] = (uint8_t)(value & 0x00ff);
}

static void mb_append_uint8(uint8_t *buf, size_t *index, uint8_t value)
{
    buf[(*index)++] = value;
}

size_t mb_encode(SENSOR_NODE_MESSAGE *msg, char *buf, size_t max)
{
    size_t index = 0;
    // GPS
    // Index    Description
    // 0 - 8    GPS timestamp
    // 9 - 16   GPS longitude
    // 17 - 24  GPS latitude
    // 25 - 32  GPS altitude
    mb_append_uint32(buf, &index, msg->gps_fix.timestamp);
    mb_append_uint32(buf, &index, msg->gps_fix.longitude);
    mb_append_uint32(buf, &index, msg->gps_fix.latitude);
    mb_append_uint32(buf, &index, msg->gps_fix.altitude);

    // Board stats
    // Index    Description
    // 33 - 40  Board temperature
    // 41 - 48  Board humidity
    mb_append_uint32(buf, &index, msg->cc2_sample.Temp_C);
    mb_append_uint32(buf, &index, msg->cc2_sample.RH);

    // AFE ADC
    // Index    Description
    // 49 - 56  OP1 ADC reading - Nitrogen Dioxide working electrode
    // 57 - 64  OP2 ADC reading - Nitrogen Dioxide auxillary electrode
    // 65 - 72  OP3 ADC reading - Ozone + Nitrogen Dioxide working electrode
    // 73 - 80  OP4 ADC reading - Ozone + Nitrogen Dioxide auxillary electrode
    // 81 - 88  OP5 ADC reading - Nitric Oxide Working electrode
    // 89 - 96  OP6 ADC reading - Nitric Oxide auxillary electrode
    // 97 - 104  Pt1000 ADC reading - AFE-3 ambient temperature
    mb_append_uint32(buf, &index, msg->afe3_sample.op1);
    mb_append_uint32(buf, &index, msg->afe3_sample.op2);
    mb_append_uint32(buf, &index, msg->afe3_sample.op3);
    mb_append_uint32(buf, &index, msg->afe3_sample.op4);
    mb_append_uint32(buf, &index, msg->afe3_sample.op5);
    mb_append_uint32(buf, &index, msg->afe3_sample.op6);
    mb_append_uint32(buf, &index, msg->afe3_sample.pt);

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
    mb_append_uint32(buf, &index, msg->opc_sample.pm_a);
    mb_append_uint32(buf, &index, msg->opc_sample.pm_b);
    mb_append_uint32(buf, &index, msg->opc_sample.pm_c);
    mb_append_uint16(buf, &index, msg->opc_sample.period);
    mb_append_uint16(buf, &index, msg->opc_sample.flowrate);
    mb_append_uint16(buf, &index, msg->opc_sample.temperature);
    mb_append_uint16(buf, &index, msg->opc_sample.fan_rev_count);
    mb_append_uint16(buf, &index, msg->opc_sample.laser_status);

    for (int i = 0; i < OPC_BINS; i++)
    {
        mb_append_uint16(buf, &index, msg->opc_sample.bin[i]);
    }
    mb_append_uint8(buf, &index, msg->opc_sample.valid);

    mb_append_s64_t(buf, &index, msg->uptime);

    return index;
}

void print_float(char *msg, float v)
{
    int n1 = floor(v);
    int n2 = floor((v - floor(v)) * 100);
    printf(msg, n1, n2);
}

void mb_dump_message(SENSOR_NODE_MESSAGE *msg)
{
    printf("Chipcap 2 / board status:\n");
    print_float("   Main board temperature: %d.%d (C)  ", msg->cc2_sample.Temp_C);
    print_float("humidity: %d.%d (%%)\n", msg->cc2_sample.RH);

    printf("GPS - status:\n");
    print_float("   Timestamp: %d.%d  ", msg->gps_fix.timestamp);
    print_float("Altitude: %d.%d  ", msg->gps_fix.altitude);
    print_float("Longitude: %d.%d (rad)  ", msg->gps_fix.longitude);
    print_float("Latitude: %f (rad)\n", msg->gps_fix.latitude);

    printf("OPC-N3 - status:\n");
    print_float("   PM A: %d.%d  ", msg->opc_sample.pm_a);
    print_float("PM B: %d.%d  ", msg->opc_sample.pm_b);
    print_float("PM C: %d.%d\n", msg->opc_sample.pm_c);
    printf("    > Period: %d\n", msg->opc_sample.period);
    printf("    > Temperature: %d\n", msg->opc_sample.temperature);
    printf("    > Humidity: %d\n", msg->opc_sample.humidity);
    printf("    > Validity: %s\n", msg->opc_sample.valid ? "YES" : "NO");
    printf("    > Histogram:\n");
    printf("    |");
    for (int i = 0; i < OPC_BINS; i++)
    {
        printf("%4d |", i);
    }
    printf("\n");
    printf("    |");
    for (int i = 0; i < OPC_BINS; i++)
    {
        printf("%4d |", msg->opc_sample.bin[i]);
    }
    printf("\n");

    printf("AFE - status:\n");
    printf("    OP1: %u  ", msg->afe3_sample.op1);
    printf("OP2: %u  ", msg->afe3_sample.op2);
    printf("OP3: %u  ", msg->afe3_sample.op3);
    printf("OP4: %u  ", msg->afe3_sample.op4);
    printf("OP5: %u  ", msg->afe3_sample.op5);
    printf("OP6: %u  ", msg->afe3_sample.op6);
    printf("PT: %u\n", msg->afe3_sample.pt);

    printf("\nUptime (seconds) : %lld\n ", msg->uptime);

    printf("----------------------------------------------------------------\n");
}
