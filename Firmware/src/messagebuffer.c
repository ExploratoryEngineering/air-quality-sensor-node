#include <zephyr.h>
#include <stdio.h>
#include <string.h>
#include "messagebuffer.h"
#include "gps_cache.h"
#include <math.h>
#include "aq.pb.h"
#include <pb_encode.h>
#include <pb_decode.h>
#include <logging/log.h>
#include "util.h"
#include "fota.h"
#include <stdlib.h>

#define LOG_LEVEL CONFIG_MESSAGEBUFFER_LOG_LEVEL
LOG_MODULE_REGISTER(MESSAGEBUFFER);

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

extern char imei[24];

#define FW_VERSION_BUF_LEN 32
char firmware_version[FW_VERSION_BUF_LEN];

uint64_t encode_firmware_version()
{
    if ((strlen(CLIENT_FIRMWARE_VER) < 5) || (strlen(CLIENT_FIRMWARE_VER) > FW_VERSION_BUF_LEN-1))
        return 0;
    memset(firmware_version, 0, FW_VERSION_BUF_LEN);
    strcpy(firmware_version, CLIENT_FIRMWARE_VER);
    
    // reto format is major.minor.patch
    int version[3] = {0,0,0};
  
    char * pMajor_end = strstr(firmware_version, ".");
    if (NULL == pMajor_end)
        return 0;
    *pMajor_end = 0;
    version[0] = atoi(firmware_version);
    pMajor_end++;
    char * pMinor_end = strstr(pMajor_end, ".");
    if (NULL == pMinor_end)
        return 0;
    *pMinor_end = 0;
    version[1] = atoi(pMajor_end);
    pMinor_end++;
    version[2] = atoi(pMinor_end);

    return ((uint64_t)version[0] << 32) | ((uint64_t)version[1] << 16) | ((uint64_t)version[2]);
}

size_t mb_encode(SENSOR_NODE_MESSAGE *msg, char *buffer, size_t max)
{
    size_t message_length = 0;
    bool status;

    memset(buffer, 0, max);

    aqpb_Sample message = aqpb_Sample_init_zero;
    pb_ostream_t stream = pb_ostream_from_buffer(buffer, max);

    message.firmware_version = encode_firmware_version();
    message.sysid = atoll(imei);

    message.uptime = msg->uptime;
    message.board_temp = msg->cc2_sample.Temp_C;
    message.board_rel_humidity = msg->cc2_sample.RH;
    // message.status == // TODO

    message.gps_timestamp = msg->gps_fix.timestamp;
    message.lat = msg->gps_fix.latitude;
    message.lon = msg->gps_fix.longitude;
    message.alt = msg->gps_fix.altitude;

    message.sensor_1_work = msg->afe3_sample.op1;
    message.sensor_1_aux = msg->afe3_sample.op2;
    message.sensor_2_work = msg->afe3_sample.op3;
    message.sensor_2_aux = msg->afe3_sample.op4;
    message.sensor_3_work = msg->afe3_sample.op5;
    message.sensor_3_aux = msg->afe3_sample.op6;
    message.afe3_temp_raw = msg->afe3_sample.pt;

    message.opc_pm_a = msg->opc_sample.pm_a;
    message.opc_pm_b = msg->opc_sample.pm_b;
    message.opc_pm_c = msg->opc_sample.pm_c;
    message.opc_sample_period = msg->opc_sample.period;
    message.opc_sample_flow_rate = msg->opc_sample.flowrate;
    message.opc_hum = msg->opc_sample.humidity;
    message.opc_temp = msg->opc_sample.temperature;
    message.opc_fan_revcount = msg->opc_sample.fan_rev_count;
    message.opc_laser_status = msg->opc_sample.laser_status;
    message.opc_sample_valid = msg->opc_sample.valid;

    message.opc_bin_0 = msg->opc_sample.bin[0];
    message.opc_bin_1 = msg->opc_sample.bin[1];
    message.opc_bin_2 = msg->opc_sample.bin[2];
    message.opc_bin_3 = msg->opc_sample.bin[3];
    message.opc_bin_4 = msg->opc_sample.bin[4];
    message.opc_bin_5 = msg->opc_sample.bin[5];
    message.opc_bin_6 = msg->opc_sample.bin[6];
    message.opc_bin_7 = msg->opc_sample.bin[7];
    message.opc_bin_8 = msg->opc_sample.bin[8];
    message.opc_bin_9 = msg->opc_sample.bin[9];
    message.opc_bin_10 = msg->opc_sample.bin[10];
    message.opc_bin_11 = msg->opc_sample.bin[11];
    message.opc_bin_12 = msg->opc_sample.bin[12];
    message.opc_bin_13 = msg->opc_sample.bin[13];
    message.opc_bin_14 = msg->opc_sample.bin[14];
    message.opc_bin_15 = msg->opc_sample.bin[15];
    message.opc_bin_16 = msg->opc_sample.bin[16];
    message.opc_bin_17 = msg->opc_sample.bin[17];
    message.opc_bin_18 = msg->opc_sample.bin[18];
    message.opc_bin_19 = msg->opc_sample.bin[19];
    message.opc_bin_20 = msg->opc_sample.bin[20];
    message.opc_bin_21 = msg->opc_sample.bin[21];
    message.opc_bin_22 = msg->opc_sample.bin[22];
    message.opc_bin_23 = msg->opc_sample.bin[23];

    status = pb_encode(&stream, aqpb_Sample_fields, &message);
    message_length = stream.bytes_written;
        
    if (!status)
    {
        LOG_ERR("Encoding failed: %s\n", log_strdup(PB_GET_ERROR(&stream)));
        return 1;
    }

    return message_length;
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
    print_float("   Main board temperature: %d (C)  ", (int)msg->cc2_sample.Temp_C);
    
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


void mb_hex_dump_message(char *buf, size_t len)
{
    printf("{");
    for (int i=0; i<len; i++)
    {
        printf("0x%02X", buf[i]);
        if (i != len-1)
            printf(", ");
    }
    printf("}");
}