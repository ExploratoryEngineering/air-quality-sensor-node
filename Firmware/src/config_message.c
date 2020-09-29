#include <zephyr.h>
#include <stdio.h>
#include <logging/log.h>
#include <misc/reboot.h>
#include "config_message.h"
#include "aqconfig.pb.h"
#include <pb_encode.h>
#include <pb_decode.h>
#include "config.h"
#include <stdlib.h>

LOG_MODULE_DECLARE(NVS);

char ping_buffer[64];

extern char imei[24];
extern apn_config CURRENT_APN_CONFIG;

int ping_id = 1;

int decode_config_message(uint8_t * buf, int size)
{
   	LOG_INF("Decoding incoming message");

    pb_istream_t stream = pb_istream_from_buffer(buf, size);
	aqconfig_setapn tmp_config = aqconfig_setapn_init_zero;

	if (!pb_decode(&stream, aqconfig_setapn_fields, &tmp_config))
	{
		LOG_ERR("protobuf decoding failed (request");
		return -1;
	}

   	LOG_INF("DECODED message");
    LOG_INF("   ID: %d", tmp_config.id);
    LOG_INF("   Timestamp: %d", tmp_config.timestamp);
    LOG_INF("   APN1: %s", log_strdup(tmp_config.apn1));
    LOG_INF("   APN2: %s", log_strdup(tmp_config.apn2));
    LOG_INF("   APN3: %s", log_strdup(tmp_config.apn3));
    LOG_INF("   COAP1: %s", log_strdup(tmp_config.coap1));
    LOG_INF("   COAP2: %s", log_strdup(tmp_config.coap2));
    LOG_INF("   COAP3: %s", log_strdup(tmp_config.coap3));

   
   if (0 != strlen(tmp_config.apn1))
		strcpy(CURRENT_APN_CONFIG.apn1,tmp_config.apn1);
	if (0 != strlen(tmp_config.apn2))
		strcpy(CURRENT_APN_CONFIG.apn2,tmp_config.apn2);
	if (0 != strlen(tmp_config.apn3))
		strcpy(CURRENT_APN_CONFIG.apn3,tmp_config.apn3);
	if (0 != strlen(tmp_config.coap1))
		strcpy(CURRENT_APN_CONFIG.coap1,tmp_config.coap1);
	if (0 != strlen(tmp_config.coap2))
		strcpy(CURRENT_APN_CONFIG.coap2,tmp_config.coap2);
	if (0 != strlen(tmp_config.coap3))
		strcpy(CURRENT_APN_CONFIG.coap3,tmp_config.coap3);

	LOG_INF("Current APN configuration is now");
    LOG_INF("Current APN1: %s", log_strdup(CURRENT_APN_CONFIG.apn1));
    LOG_INF("Current APN2: %s", log_strdup(CURRENT_APN_CONFIG.apn2));
    LOG_INF("Current APN3: %s", log_strdup(CURRENT_APN_CONFIG.apn3));
    LOG_INF("Current APN4: %s", log_strdup(CURRENT_APN_CONFIG.apn4));
    LOG_INF("Current COAP1: %s", log_strdup(CURRENT_APN_CONFIG.coap1));
    LOG_INF("Current COAP2: %s", log_strdup(CURRENT_APN_CONFIG.coap2));
    LOG_INF("Current COAP3: %s", log_strdup(CURRENT_APN_CONFIG.coap3));
    LOG_INF("Current COAP4: %s", log_strdup(CURRENT_APN_CONFIG.coap4));		

    save_apn_config();

    LOG_INF("Scheduling reboot");
    k_sleep(3000);
    return 0;
}

int encode_ping(char * buffer, int buffer_size)
{
    aqconfig_ping ping = aqconfig_ping_init_zero;
    pb_ostream_t stream = pb_ostream_from_buffer(buffer, buffer_size);
    
    ping.id = ping_id++;
    ping.deviceid = atoll(imei);
    ping.timestamp = k_uptime_get() / 1000; // Uptime in seconds == kinda time'ish

    bool status = pb_encode(&stream, aqconfig_ping_fields, &ping);
        
    if (!status)
    {
        LOG_ERR("PING Encoding failed: %s\n", log_strdup(PB_GET_ERROR(&stream)));
        return -1;
    }

    return 0;
}
