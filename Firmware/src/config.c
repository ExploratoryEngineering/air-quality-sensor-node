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

#define NVS_FOTA_COAP_SERVER_ID 1
#define NVS_FOTA_COAP_PORT_ID 5
#define NVS_FOTA_COAP_UPDATE_PATH_ID 9
#define NVS_FOTA_COAP_REPORT_PATH_ID 13
#define NVS_APN_NAME_ID 17

// Errors
#define INVALID_APN_PARAMETER -1
#define INVALID_COAP_PARAMETER -2

// Default APN list
char DEFAULT_APN[4][CONFIG_NAME_SIZE] = {"mda.lab5e", "mda.ee", "telenor.iotgw", "telenor.iot"};

// Default COAP parameters
#define DEFAULT_FOTA_COAP_SERVER "172.16.15.14"
#define DEFAULT_FOTA_COAP_REPORT_PATH  "u"
#define DEFAULT_FOTA_COAP_UPDATE_PATH	 "fw"
#define DEFAULT_FOTA_COAP_PORT 5683

// Global variables
char APN_NAME[NVS_APN_COUNT][CONFIG_NAME_SIZE];

char FOTA_COAP_SERVER[NVS_APN_COUNT][CONFIG_NAME_SIZE];
char FOTA_COAP_REPORT_PATH[NVS_APN_COUNT][CONFIG_NAME_SIZE];
char FOTA_COAP_UPDATE_PATH[NVS_APN_COUNT][CONFIG_NAME_SIZE];

int FOTA_COAP_PORT[NVS_APN_COUNT];
int ACTIVE_APN_INDEX = 0;

extern decoded_config_value decoded_values[APN_COMMAND_ARGUMENTS];

int save_new_apn_config(int argument_count)
{
	for (int i=0; i<argument_count; i++)
    {
			char * stringval = decoded_values[i].string_val;
			int length = strlen(stringval)+1;
			int intval = decoded_values[i].int_val;
			if (decoded_values[i].id == 1) // First APN
			{
				nvs_write(&fs, NVS_APN_NAME_ID, stringval, length);
			}
			else if (decoded_values[i].id == 2) // Second APN)
			{
				nvs_write(&fs, NVS_APN_NAME_ID+1, stringval, length);
			}
			else if (decoded_values[i].id == 3) // Third APN)
			{
				nvs_write(&fs, NVS_APN_NAME_ID+2, stringval, length);
			}

			else if (decoded_values[i].id == 10) // First COAP server)
			{
				nvs_write(&fs, NVS_FOTA_COAP_SERVER_ID, stringval, length);
			}
			else if (decoded_values[i].id == 11) // First COAP port)
			{
				nvs_write(&fs, NVS_FOTA_COAP_PORT_ID, &intval, sizeof(intval));
			}
			else if (decoded_values[i].id == 12) // First COAP update path)
			{
				nvs_write(&fs, NVS_FOTA_COAP_UPDATE_PATH_ID, stringval, length);
			}

			else if (decoded_values[i].id == 13) // Second COAP server)
			{
				nvs_write(&fs, NVS_FOTA_COAP_SERVER_ID+1, stringval, length);
			}
			else if (decoded_values[i].id == 14) // Second COAP port)
			{
				nvs_write(&fs, NVS_FOTA_COAP_PORT_ID+1, &intval, sizeof(intval));
			}
			else if (decoded_values[i].id == 15) // Second COAP update path)
			{
				nvs_write(&fs, NVS_FOTA_COAP_UPDATE_PATH_ID+1, stringval, length);
			}

			else if (decoded_values[i].id == 16) // Third COAP server)
			{
				nvs_write(&fs, NVS_FOTA_COAP_SERVER_ID+2, stringval, length);
			}
			else if (decoded_values[i].id == 17) // Third COAP port)
			{
				nvs_write(&fs, NVS_FOTA_COAP_PORT_ID+2, &intval, sizeof(intval));
			}
			else if (decoded_values[i].id == 18) // Third COAP update path)
			{
				nvs_write(&fs, NVS_FOTA_COAP_UPDATE_PATH_ID+2, stringval, length);
			}
    }
		return 0;
}

void select_active_apn()
{
	LOG_INF("Checking APN connectivity");

	// ACTIVE_APN_INDEX is global. Careful where you t(h)read
	for (ACTIVE_APN_INDEX=0; ACTIVE_APN_INDEX<NVS_APN_COUNT; ACTIVE_APN_INDEX++)
	{
		modem_restart();
		modem_configure();
		int retries = 0;
		while (!modem_is_ready() & (retries++ < APN_QUERY_RETRY_COUNT))
		{
			LOG_INF("Waiting for IP-address from %s. Retry : %d", log_strdup(APN_NAME[ACTIVE_APN_INDEX]), retries);
			k_sleep(APN_RETRY_DELAY_MS);
		}
		if (modem_is_ready())
		{
			LOG_INF("SELECTING APN : %s", log_strdup(APN_NAME[ACTIVE_APN_INDEX]));
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

void write_default_values_to_flash()
{
	LOG_INF("Populating default APN list in flash");
	for (int i=0; i<NVS_APN_COUNT; i++)
	{
		LOG_INF("Writing default APN %s (id:%d)", log_strdup(DEFAULT_APN[i]), NVS_APN_NAME_ID+i);
		nvs_write(&fs, NVS_APN_NAME_ID+i, 	&DEFAULT_APN[i], strlen(DEFAULT_APN[i])+1);
	}

	for (int i=0; i<NVS_APN_COUNT; i++)
	{
		strcpy(FOTA_COAP_SERVER[i], DEFAULT_FOTA_COAP_SERVER);
		strcpy(FOTA_COAP_REPORT_PATH[i], DEFAULT_FOTA_COAP_REPORT_PATH);
		strcpy(FOTA_COAP_UPDATE_PATH[i], DEFAULT_FOTA_COAP_UPDATE_PATH);
		FOTA_COAP_PORT[i] = DEFAULT_FOTA_COAP_PORT;

		LOG_INF("Writing default COAP Server %s (id:%d)", log_strdup(FOTA_COAP_SERVER[i]), i+1);
		LOG_INF("Writing default COAP Report path %s (id:%d)", log_strdup(FOTA_COAP_REPORT_PATH[i]), i+1);
		LOG_INF("Writing default COAP Update path %s (id:%d)", log_strdup(FOTA_COAP_UPDATE_PATH[i]), i+1);
		LOG_INF("Writing default COAP Port %d (id:%d)", FOTA_COAP_PORT[i], i+1);

		nvs_write(&fs, NVS_FOTA_COAP_SERVER_ID+i, 			FOTA_COAP_SERVER[i], strlen(FOTA_COAP_SERVER[i])+1);
		nvs_write(&fs, NVS_FOTA_COAP_REPORT_PATH_ID+i, 	FOTA_COAP_REPORT_PATH[i], strlen(FOTA_COAP_REPORT_PATH[i])+1);
		nvs_write(&fs, NVS_FOTA_COAP_UPDATE_PATH_ID+i, 	FOTA_COAP_UPDATE_PATH[i], strlen(FOTA_COAP_UPDATE_PATH[i])+1);
		nvs_write(&fs, NVS_FOTA_COAP_PORT_ID+i, 				&FOTA_COAP_PORT[i], sizeof(int));
	}
}

bool missing_flash_parameter()
{
	// Check APNs
	for (int i=0; i<NVS_APN_COUNT; i++)
	{
		int rc = nvs_read(&fs, NVS_APN_NAME_ID + i, &APN_NAME[i], CONFIG_NAME_SIZE);
		if (rc > 0) 
		{  
			LOG_INF("APN Name %d read ok from FLASH : %s", i+1, log_strdup(APN_NAME[i]));
		} else   
		{
			LOG_INF("Missing APN name at offset :%d", i);
			return true;
		}
	}

	// Check COAP servers
	for (int i=0; i<NVS_APN_COUNT; i++)
	{
		int rc = nvs_read(&fs, NVS_FOTA_COAP_SERVER_ID + i, &FOTA_COAP_SERVER[i], CONFIG_NAME_SIZE);
		if (rc > 0) 
		{  
			LOG_INF("COAP Server %d read ok from FLASH : %s", i+1, log_strdup(FOTA_COAP_SERVER[i]));
		} else   
		{
			LOG_INF("Missing COAP Server name at offset :%d", i);
			return true;
		}
	}

	// Check COAP Report paths
	for (int i=0; i<NVS_APN_COUNT; i++)
	{
		int rc = nvs_read(&fs, NVS_FOTA_COAP_REPORT_PATH_ID + i, &FOTA_COAP_REPORT_PATH[i], CONFIG_NAME_SIZE);
		if (rc > 0) 
		{  
			LOG_INF("COAP Report path %d read ok from FLASH : %s", i+1, log_strdup(FOTA_COAP_REPORT_PATH[i]));
		} else   
		{
			LOG_INF("Missing COAP Report path at offset :%d", i);
			return true;
		}
	}

	// Check COAP Update paths
	for (int i=0; i<NVS_APN_COUNT; i++)
	{
		int rc = nvs_read(&fs, NVS_FOTA_COAP_UPDATE_PATH_ID + i, &FOTA_COAP_UPDATE_PATH[i], CONFIG_NAME_SIZE);
		if (rc > 0) 
		{  
			LOG_INF("COAP Update path %d read ok from FLASH : %s", i+1, log_strdup(FOTA_COAP_UPDATE_PATH[i]));
		} else   
		{
			LOG_INF("Missing COAP Update path at offset :%d", i);
			return true;
		}
	}
	
	// Check COAP ports
	for (int i=0; i<NVS_APN_COUNT; i++)
	{
		int rc = nvs_read(&fs, NVS_FOTA_COAP_PORT_ID + i, &FOTA_COAP_PORT[i], sizeof(int));
		if (rc > 0) 
		{  
			LOG_INF("COAP Update port %d read ok from FLASH : %d", i+1, FOTA_COAP_PORT[i]);
		} else   
		{
			LOG_INF("Missing COAP PORT at offset :%d", i);
			return true;
		}
	}

	return false;
}

/**
 * Checks for the existense of a predefined APN list in FLASH memory
 * If the list is missing or incomplete, a default list consisting of
 * the names defined by DEFAULT_APN_1, DEFAULT_APN_2, DEFAULT_APN_3 
 * is saved to FLASH memory
 * 
 * Must be called after init_config_nvs();
 */
void init_default_apn_list()
{
	LOG_INF("Scanning stored APN NAMES in flash");

	if (missing_flash_parameter())
	{
		write_default_values_to_flash();
	}
}


void init_config_nvs()
{
	struct flash_pages_info info;
	int rc = 0;

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

	init_default_apn_list();

	if (!missing_flash_parameter())
	{
		LOG_INF("Flash parameter list is ok.");
	}
	
	// We will have to select_active_apn() after the rest of the hardware has been initialized.
	// At dawn, look to main()
}


