#ifndef _CONFIG_H_
#define _CONFIG_H_

#define APN_QUERY_RETRY_COUNT 	20
#define APN_RETRY_DELAY_MS		2000	

#define PING_RETRIES    20
#define PING_RETRY_DELAY_MS		2000	


#define NVS_APN_COUNT 4

// Default COAP parameters
#define DEFAULT_FOTA_COAP_REPORT_PATH   "u"
#define DEFAULT_FOTA_COAP_UPDATE_PATH   "fw"
#define DEFAULT_FOTA_COAP_PORT 5683


// Force reboot after 2 hours. This wil trigger a new scan for active APNs
#define UPTIME_FORCE_REBOOT_LIMIT_SECONDS 60 * 120

#include "config_message.h"
#include "aqconfig.pb.h"

void init_config_nvs();
void select_active_apn();
bool select_ping_apn();
int save_apn_config();

#endif // _CONFIG_H_