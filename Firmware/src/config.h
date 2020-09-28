#ifndef _CONFIG_H_
#define _CONFIG_H_

#define FOTA_COAP_PORT_SIZE sizeof(int)
#define CONFIG_NAME_SIZE 64

#define APN_QUERY_RETRY_COUNT 	20
#define APN_RETRY_DELAY_MS		2000	

#define NVS_APN_COUNT 4

// Force reboot after 24 hours. This wil trigger a new scan for active APNs
#define UPTIME_FORCE_REBOOT_LIMIT_SECONDS 60 * 60 * 24 


#include "config_message.h"


void init_config_nvs();
void select_active_apn();
int save_new_apn_config(int argument_count);

#endif // _CONFIG_H_