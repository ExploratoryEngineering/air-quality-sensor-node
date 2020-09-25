#ifndef _CONFIG_H_
#define _CONFIG_H_

#include "config.pb.h"

#define FOTA_COAP_SERVER_SIZE 256
#define FOTA_COAP_PORT_SIZE sizeof(int)
#define FOTA_COAP_REPORT_PATH_SIZE 256
#define APN_NAME_SIZE 64

#define APN_QUERY_RETRY_COUNT 	20
#define APN_RETRY_DELAY_MS		2000	

#define NVS_APN_COUNT 3

// Force reboot after 24 hours. This wil trigger a new scan for active APNs
#define UPTIME_FORCE_REBOOT_LIMIT_SECONDS 60 * 60 * 24 

void init_config_nvs();
void select_active_apn();

#endif // _CONFIG_H_