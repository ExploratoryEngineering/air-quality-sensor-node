    #include <zephyr.h>
#include <logging/log.h>
#include <stdio.h>
#include "messagebuffer.h"
#include "gps_cache.h"

SENSOR_NODE_MESSAGE sensor_node_message;
uint8_t TRANSMIT_BUFFER_TEXT[TRANSMIT_BUFFER_TEXT_SIZE];

char tmpBuf[256];

void zepyr_annoying_printk(char * msg)
{
    printk("%s", msg);
    k_sleep(50);
}

void encodeUint32Value(uint8_t * buffer, unsigned int value)
{
    sprintf(buffer, "%02X%02X%02X%02X", (value & 0xff000000) >> 24, (value & 0x00ff0000) >> 16, (value & 0x0000ff00) >> 8, (value & 0x000000ff));
}

void encodeUint16Value(uint8_t * buffer, unsigned int value)
{
    sprintf(buffer, "%02X%02X", (value & 0x0000ff00) >> 8, (value & 0x000000ff));
}

void encodeUint8Value(uint8_t * buffer, unsigned int value)
{
    sprintf(buffer, "%02X", value);
}

void append_encoded_32(unsigned int value)
{
    encodeUint32Value(tmpBuf, value);
    strcat(TRANSMIT_BUFFER_TEXT, tmpBuf);
}

void append_encoded_16(unsigned int value)
{
    encodeUint16Value(tmpBuf, value);
    strcat(TRANSMIT_BUFFER_TEXT, tmpBuf);
}

void append_encoded_8(unsigned int value)
{
    encodeUint8Value(tmpBuf, value);
    strcat(TRANSMIT_BUFFER_TEXT, tmpBuf);
}



uint8_t * encodeNBIotMessage()
{
    // Format is ASCII rerpresentation of hex
    memset(TRANSMIT_BUFFER_TEXT, 0, TRANSMIT_BUFFER_TEXT_SIZE);

    // GPS
    // Index    Description
    // 0 - 8    GPS timestamp
    // 9 - 16   GPS longitude
    // 17 - 24  GPS latitude
    // 25 - 32  GPS altitude
    append_encoded_32(sensor_node_message.sample.gps_fix.timestamp);
    append_encoded_32(sensor_node_message.sample.gps_fix.longitude);
    append_encoded_32(sensor_node_message.sample.gps_fix.latitude);
    append_encoded_32(sensor_node_message.sample.gps_fix.altitude);

    // Board stats
    // Index    Description
    // 33 - 40  Board temperature
    // 41 - 48  Board humidity
    append_encoded_32(sensor_node_message.sample.cc2_sample.Temp_C);
    append_encoded_32(sensor_node_message.sample.cc2_sample.RH);

    // AFE ADC
    // Index    Description
    // 49 - 56  OP1 ADC reading - Nitrogen Dioxide working electrode
    // 57 - 64  OP2 ADC reading - Nitrogen Dioxide auxillary electrode
    // 65 - 72  OP3 ADC reading - Ozone + Nitrogen Dioxide working electrode
    // 73 - 80  OP4 ADC reading - Ozone + Nitrogen Dioxide auxillary electrode
    // 81 - 88  OP5 ADC reading - Nitric Oxide Working electrode
    // 89 - 96  OP6 ADC reading - Nitric Oxide auxillary electrode
    // 97 - 104  Pt1000 ADC reading - AFE-3 ambient temperature
    append_encoded_32(sensor_node_message.sample.afe3_sample.op1);
    append_encoded_32(sensor_node_message.sample.afe3_sample.op2);
    append_encoded_32(sensor_node_message.sample.afe3_sample.op3);
    append_encoded_32(sensor_node_message.sample.afe3_sample.op4);
    append_encoded_32(sensor_node_message.sample.afe3_sample.op5);
    append_encoded_32(sensor_node_message.sample.afe3_sample.op6);
    append_encoded_32(sensor_node_message.sample.afe3_sample.pt);

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
    append_encoded_32(sensor_node_message.sample.opc_sample.pm_a);
    append_encoded_32(sensor_node_message.sample.opc_sample.pm_b);
    append_encoded_32(sensor_node_message.sample.opc_sample.pm_c);
    append_encoded_16(sensor_node_message.sample.opc_sample.period);
    append_encoded_16(sensor_node_message.sample.opc_sample.flowrate);
    append_encoded_16(sensor_node_message.sample.opc_sample.temperature);
    append_encoded_16(sensor_node_message.sample.opc_sample.fan_rev_count);
    append_encoded_16(sensor_node_message.sample.opc_sample.laser_status);

    for (int i=0; i<OPC_BINS; i++) 
    {
        append_encoded_16(sensor_node_message.sample.opc_sample.bin[i]);
    }
    append_encoded_8(sensor_node_message.sample.opc_sample.valid);

    return &TRANSMIT_BUFFER_TEXT[0];
}



void DEBUG_CC2()
{
    zepyr_annoying_printk("Chipcap 2 / board status:\n");
    sprintf(tmpBuf, "   > Main board temperature: %.2f (C)\n", sensor_node_message.sample.cc2_sample.Temp_C);
    zepyr_annoying_printk(tmpBuf);
    sprintf(tmpBuf, "   > Main board humidity: %.2f (%%)\n", sensor_node_message.sample.cc2_sample.RH);
    zepyr_annoying_printk(tmpBuf);
}

void DEBUG_GPS()
{
    zepyr_annoying_printk("GPS - status:\n");

    sprintf(tmpBuf, "   > Timestamp: %f\n", sensor_node_message.sample.gps_fix.timestamp);
    zepyr_annoying_printk(tmpBuf);
    sprintf(tmpBuf, "   > Altitude: %f \n", sensor_node_message.sample.gps_fix.altitude);
    zepyr_annoying_printk(tmpBuf);
    sprintf(tmpBuf, "   > Longitude: %f (rad)\n", sensor_node_message.sample.gps_fix.longitude);
    zepyr_annoying_printk(tmpBuf);
    sprintf(tmpBuf, "   > Latitude: %f (rad)\n", sensor_node_message.sample.gps_fix.latitude);
    zepyr_annoying_printk(tmpBuf);
}

void DEBUG_OPC()
{
    zepyr_annoying_printk("OPC-N3 - status:\n");
    sprintf(tmpBuf, "   > PM A: %f\n", sensor_node_message.sample.opc_sample.pm_a);
    zepyr_annoying_printk(tmpBuf);
    sprintf(tmpBuf, "   > PM B: %f\n", sensor_node_message.sample.opc_sample.pm_b);
    zepyr_annoying_printk(tmpBuf);
    sprintf(tmpBuf, "   > PM C: %f\n", sensor_node_message.sample.opc_sample.pm_c);
    zepyr_annoying_printk(tmpBuf);
    sprintf(tmpBuf, "    > Period: %d\n", sensor_node_message.sample.opc_sample.period);
    zepyr_annoying_printk(tmpBuf);
    sprintf(tmpBuf, "    > Temperature: %d\n", sensor_node_message.sample.opc_sample.temperature);
    zepyr_annoying_printk(tmpBuf);
    sprintf(tmpBuf, "    > Humidity: %d\n", sensor_node_message.sample.opc_sample.humidity);
    zepyr_annoying_printk(tmpBuf);
    sprintf(tmpBuf, "    > Validity: %s\n", sensor_node_message.sample.opc_sample.valid ? "YES" : "NO");
    zepyr_annoying_printk(tmpBuf);
    zepyr_annoying_printk("    > Histogram:\n");
    for (int i=0; i<OPC_BINS; i++) 
    {
        sprintf(tmpBuf, "        bin%02d : %d\n", i, sensor_node_message.sample.opc_sample.bin[i]);
        zepyr_annoying_printk(tmpBuf);
    }
}

void DEBUG_AFE()
{
    zepyr_annoying_printk("AFE - status:\n");
    sprintf(tmpBuf, "    > OP1: %u\n", sensor_node_message.sample.afe3_sample.op1);
    zepyr_annoying_printk(tmpBuf);
    sprintf(tmpBuf, "    > OP2: %u\n", sensor_node_message.sample.afe3_sample.op2);
    zepyr_annoying_printk(tmpBuf);
    sprintf(tmpBuf, "    > OP3: %u\n", sensor_node_message.sample.afe3_sample.op3);
    zepyr_annoying_printk(tmpBuf);
    sprintf(tmpBuf, "    > OP4: %u\n", sensor_node_message.sample.afe3_sample.op4);
    zepyr_annoying_printk(tmpBuf);
    sprintf(tmpBuf, "    > OP5: %u\n", sensor_node_message.sample.afe3_sample.op5);
    zepyr_annoying_printk(tmpBuf);
    sprintf(tmpBuf, "    > OP6: %u\n", sensor_node_message.sample.afe3_sample.op6);
    zepyr_annoying_printk(tmpBuf);
    sprintf(tmpBuf, "    > PT: %u\n", sensor_node_message.sample.afe3_sample.pt);
    zepyr_annoying_printk(tmpBuf);
}
