#include <zephyr.h>
#include <stdio.h>
#include "config.h"
#include "comms.h"
#include <drivers/flash.h>
#include <fs/nvs.h>
#include <device.h>
#include <misc/reboot.h>
#include "config_message.h"


#include <logging/log.h>
LOG_MODULE_REGISTER(NVS, CONFIG_NVS_LOG_LEVEL);

static struct nvs_fs fs;

// Handle for NVS file system
#define NVS_APN_NAME_ID	1

apn_config CURRENT_APN_CONFIG;
char CURRENT_APN_BUFFER[64] = {0};
char CURRENT_COAP_BUFFER[128] = {0};

int save_apn_config()
{
	LOG_INF("Saving new APN configuration to flash.");
  LOG_INF("Saving APN1: %s", log_strdup(CURRENT_APN_CONFIG.apn1));
  LOG_INF("Saving APN2: %s", log_strdup(CURRENT_APN_CONFIG.apn2));
  LOG_INF("Saving APN3: %s", log_strdup(CURRENT_APN_CONFIG.apn3));
  LOG_INF("Saving APN4: %s", log_strdup(CURRENT_APN_CONFIG.apn4));
  LOG_INF("Saving COAP1: %s", log_strdup(CURRENT_APN_CONFIG.coap1));
  LOG_INF("Saving COAP2: %s", log_strdup(CURRENT_APN_CONFIG.coap2));
  LOG_INF("Saving COAP3: %s", log_strdup(CURRENT_APN_CONFIG.coap3));
  LOG_INF("Saving COAP4: %s", log_strdup(CURRENT_APN_CONFIG.coap4));

	int b = nvs_write(&fs, NVS_APN_NAME_ID, &CURRENT_APN_CONFIG, sizeof(CURRENT_APN_CONFIG));

  LOG_INF("%d bytes written to flash.", b);
	return b;
}

void select_active_apn()
{
	LOG_INF("Checking APN connectivity");

	char * pAPN = NULL;
	char * pCOAP = NULL;

	for (int i =0; i<NVS_APN_COUNT; i++)
	{
		switch (i)
		{
				case 0: pAPN = CURRENT_APN_CONFIG.apn1;
						    pCOAP = CURRENT_APN_CONFIG.coap1;
								break;
				case 1: pAPN = CURRENT_APN_CONFIG.apn2;
						    pCOAP = CURRENT_APN_CONFIG.coap2;
								break;
				case 2: pAPN = CURRENT_APN_CONFIG.apn3;
						    pCOAP = CURRENT_APN_CONFIG.coap3;
								break;
				case 3: pAPN = CURRENT_APN_CONFIG.apn4;
						    pCOAP = CURRENT_APN_CONFIG.coap4;
								break;
				default: LOG_ERR("Hey! There is a house limit of 3 APNs (+ a secret one). Brace for bus error!");
								k_sleep(1000);
								break;
		}
		strcpy(CURRENT_APN_BUFFER, pAPN);
		strcpy(CURRENT_COAP_BUFFER, pCOAP);

		LOG_INF("Testing APN : %s, COAP: %s", log_strdup(CURRENT_APN_BUFFER), log_strdup(CURRENT_COAP_BUFFER));

		modem_restart();
		modem_configure();
		int retries = 0;
		while (!modem_is_ready() & (retries++ < APN_QUERY_RETRY_COUNT))
		{
			LOG_INF("Waiting for IP-address from %s. Retry : %d", log_strdup(CURRENT_APN_BUFFER), retries);
			k_sleep(APN_RETRY_DELAY_MS);
		}
		if (modem_is_ready())
		{
			LOG_INF("SELECTING APN : %s", log_strdup(CURRENT_APN_BUFFER));
			return; 
		}
	}
	// You really don't want to end up here, but it _can_ happen if a all (previously active) APNs have died
	LOG_ERR("This device is effectively bricked. Going to sleep and rebooting later");

	// But don't despair, We'll take a power nap. Wait for better days. Reboot and try again. (Depending on watchdog feeding
	// and watchdog timeout, a reboot may happen sooner)
	k_sleep(1000*60*60); 
	sys_reboot(0);
}

void init_default_config()
{
		LOG_INF("Initializing default APN config");
	  strcpy(CURRENT_APN_CONFIG.apn4, "mda.lab5e");
    strcpy(CURRENT_APN_CONFIG.apn2, "mda.ee");
    strcpy(CURRENT_APN_CONFIG.apn3, "telenor.iotgw");
    strcpy(CURRENT_APN_CONFIG.apn1, "telenor.iot");

    strcpy(CURRENT_APN_CONFIG.coap4, "172.16.15.14");
    strcpy(CURRENT_APN_CONFIG.coap2, "172.16.15.14");
    strcpy(CURRENT_APN_CONFIG.coap3, "172.16.32.1");
    strcpy(CURRENT_APN_CONFIG.coap1, "88.99.192.151");
	/*
	  strcpy(CURRENT_APN_CONFIG.apn1, "mda.lab5e");
    strcpy(CURRENT_APN_CONFIG.apn2, "mda.ee");
    strcpy(CURRENT_APN_CONFIG.apn3, "telenor.iotgw");
    strcpy(CURRENT_APN_CONFIG.apn4, "telenor.iot");

    strcpy(CURRENT_APN_CONFIG.coap1, "172.16.15.14");
    strcpy(CURRENT_APN_CONFIG.coap2, "172.16.15.14");
    strcpy(CURRENT_APN_CONFIG.coap3, "172.16.32.1");
    strcpy(CURRENT_APN_CONFIG.coap4, "88.99.192.151");
*/		
}

bool has_flash_config()
{
	apn_config tmp_config;
	int rc = nvs_read(&fs, NVS_APN_NAME_ID, &tmp_config, sizeof(tmp_config));
	if (rc > 0) 
	{  
		LOG_INF("APN Configuration read from FLASH");
	} else   
	{
		LOG_INF("Missing APN configuration in FLASH");
		return false;
	}

	if (0 != strcmp(tmp_config.apn4, CURRENT_APN_CONFIG.apn4))
		return false;
	if (0 != strcmp(tmp_config.coap4, CURRENT_APN_CONFIG.coap4))
		return false;

	LOG_INF("Read APN configuration from flash.");
  LOG_INF("Read APN1: %s", log_strdup(tmp_config.apn1));
  LOG_INF("Read APN2: %s", log_strdup(tmp_config.apn2));
  LOG_INF("Read APN3: %s", log_strdup(tmp_config.apn3));
  LOG_INF("Read APN4: %s", log_strdup(tmp_config.apn4));
  LOG_INF("Read COAP1: %s", log_strdup(tmp_config.coap1));
  LOG_INF("Read COAP2: %s", log_strdup(tmp_config.coap2));
  LOG_INF("Read COAP3: %s", log_strdup(tmp_config.coap3));
  LOG_INF("Read COAP4: %s", log_strdup(tmp_config.coap4));

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

	return true;
}

void init_config_nvs()
{
	struct flash_pages_info info;
	int rc = 0;

	init_default_config();

	// Define the nvs file system by settings with:
	// 	- sector_size equal to the pagesize,
	//  - 3 sectors
	//  - starting at DT_FLASH_AREA_STORAGE_OFFSET
	fs.offset = DT_FLASH_AREA_STORAGE_OFFSET;
	rc = flash_get_page_info_by_offs(device_get_binding(DT_FLASH_DEV_NAME), fs.offset, &info);
	if (rc) 
	{
		// This is bad...
		LOG_ERR("Unable to get page info");
	}
	fs.sector_size = info.size;
	fs.sector_count = 3U;

	rc = nvs_init(&fs, DT_FLASH_DEV_NAME);
	if (rc) 
	{
		// This is also bad...
		LOG_ERR("Flash Init failed\n");
	}

  // nvs_clear(&fs); // Nuke the file system

	size_t freespace = nvs_calc_free_space(&fs);
 	LOG_INF("Calculated free space: %d", freespace);

	if (!has_flash_config())
	 	save_apn_config();
}


