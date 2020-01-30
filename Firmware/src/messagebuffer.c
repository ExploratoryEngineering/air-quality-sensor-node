    #include <zephyr.h>
#include <logging/log.h>
#include <stdio.h>
#include "messagebuffer.h"
#include "gps_cache.h"

SENSOR_NODE_MESSAGE sensor_node_message;

char tmpBuf[256];

void zepyr_annoying_printk(char * msg)
{
    printk("%s", msg);
    k_sleep(50);
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

