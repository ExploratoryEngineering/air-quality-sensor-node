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
#define NVS_FOTA_COAP_PORT_ID 2
#define NVS_FOTA_COAP_REPORT_PATH_ID 3
#define NVS_APN_NAME_ID 4

// Errors
#define INVALID_APN_PARAMETER -1
#define INVALID_COAP_PARAMETER -2

// Default APN list
#define DEFAULT_APN_1 "mda.lab5e"
#define DEFAULT_APN_2 "mda.ee"
#define DEFAULT_APN_3 "telenor.iotgw"
#define DEFAULT_APN_4 "telenor.iotgw"

// Default COAP parameters
#define DEFAULT_FOTA_COAP_SERVER "172.16.15.14"
#define DEFAULT_FOTA_COAP_REPORT_PATH  "u"
#define DEFAULT_FOTA_COAP_PORT 5683

// Global variables
char FOTA_COAP_SERVER[FOTA_COAP_SERVER_SIZE];
int FOTA_COAP_PORT;
char FOTA_COAP_REPORT_PATH[FOTA_COAP_REPORT_PATH_SIZE];
char APN_NAME[NVS_APN_COUNT][APN_NAME_SIZE];
int ACTIVE_APN_INDEX = 0;


void save_new_apn_config(set_apn_list_command cmd)
{
	LOG_INF("Storing new APN list in flash")
	nvs_write(&fs, NVS_APN_NAME_ID, cmd.apn[0], strlen(cmd.apn[0])+1);
	nvs_write(&fs, NVS_APN_NAME_ID+1, cmd.apn[1], strlen(cmd.apn[1])+1);
	nvs_write(&fs, NVS_APN_NAME_ID+2, cmd.apn[2], strlen(cmd.apn[2])+1);
}

void save_new_coap_config()
{
	#pragma message("TODO: Implement save_new_coap_config()")
	// TODO:
	//		- function arguments
	//		- decide on argument validation rules
	// 		- validation (validate attribute or ruleset)
	//		- save to NVS or discard
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

	bool missing_apn = false;
	for (int apn_index=0; apn_index<NVS_APN_COUNT; apn_index++)
	{
		int rc = nvs_read(&fs, NVS_APN_NAME_ID + apn_index, &APN_NAME[apn_index], APN_NAME_SIZE);
		if (rc > 0) 
		{  
			LOG_INF("Found APN Name at offset %d in FLASH : %s", apn_index, log_strdup(APN_NAME[apn_index]));
		} else   
		{
			LOG_INF("Missing APN name at offset :%d", apn_index);
			missing_apn = true;
		}
	}

	if (missing_apn)
	{
		LOG_INF("Populating default APN list in flash");
		nvs_write(&fs, NVS_APN_NAME_ID, &DEFAULT_APN_1, strlen(DEFAULT_APN_1)+1);
		nvs_write(&fs, NVS_APN_NAME_ID+1, &DEFAULT_APN_2, strlen(DEFAULT_APN_2)+1);
		nvs_write(&fs, NVS_APN_NAME_ID+2, &DEFAULT_APN_3, strlen(DEFAULT_APN_3)+1);
		nvs_write(&fs, NVS_APN_NAME_ID+3, &DEFAULT_APN_4, strlen(DEFAULT_APN_4)+1);
	}
}

/**
 * Checks for the existense of predefined COAP parametersin FLASH memory
 * If parameters are incomplete or missing, they are created from the
 * definitions in DEFAULT_FOTA_COAP_SERVER, DEFAULT_FOTA_COAP_REPORT_PATH, DEFAULT_FOTA_COAP_PORT
 * and saved to FLASH memory
 * 
 * Must be called after init_config_nvs();
  */
void init_default_COAP_parameters()
{
	// Check if the Coap server is stored in flash. If not. Initialize it with the default value
	int rc = nvs_read(&fs, NVS_FOTA_COAP_SERVER_ID, &FOTA_COAP_SERVER, sizeof(FOTA_COAP_SERVER));
	if (rc > 0) 
	{  
		LOG_INF("COAP Server read ok from FLASH : %s", log_strdup(FOTA_COAP_SERVER));
	} else   
	{
		strcpy(FOTA_COAP_SERVER, DEFAULT_FOTA_COAP_SERVER);
		LOG_INF("COAP Server NOT FOUND in FLASH storage. Defaulting to : %s", log_strdup(FOTA_COAP_SERVER));
		LOG_INF("Writing default APN name to flash storage");
		nvs_write(&fs, NVS_FOTA_COAP_SERVER_ID, &FOTA_COAP_SERVER, strlen(FOTA_COAP_SERVER)+1);
	}

	// Check if the Coap report path is stored in flash. If not. Initialize it with the default value
	rc = nvs_read(&fs, NVS_FOTA_COAP_REPORT_PATH_ID, &FOTA_COAP_REPORT_PATH, sizeof(FOTA_COAP_REPORT_PATH));
	if (rc > 0) 
	{  
		LOG_INF("COAP Report path read ok from FLASH : %s", log_strdup(FOTA_COAP_REPORT_PATH));
	} else   
	{
		strcpy(FOTA_COAP_REPORT_PATH, DEFAULT_FOTA_COAP_REPORT_PATH);
		LOG_INF("COAP report path NOT FOUND in FLASH storage. Defaulting to : %s", log_strdup(FOTA_COAP_REPORT_PATH));
		LOG_INF("Writing default coap report path to flash storage");
		nvs_write(&fs, NVS_FOTA_COAP_REPORT_PATH_ID, &FOTA_COAP_REPORT_PATH, strlen(FOTA_COAP_REPORT_PATH)+1);
	}

	// Check if the Coap port is stored in flash. If not. Initialize it with the default value
	rc = nvs_read(&fs, NVS_FOTA_COAP_PORT_ID, &FOTA_COAP_PORT, sizeof(FOTA_COAP_PORT));
	if (rc > 0) 
	{  
		LOG_INF("COAP port read ok from FLASH : %d", FOTA_COAP_PORT);
	} else   
	{
		FOTA_COAP_PORT = DEFAULT_FOTA_COAP_PORT;;
		LOG_INF("COAP port NOT FOUND in FLASH storage. Defaulting to : %d", FOTA_COAP_PORT);
		LOG_INF("Writing default coap port to flash storage");
		nvs_write(&fs, NVS_FOTA_COAP_PORT_ID, &FOTA_COAP_PORT, sizeof(FOTA_COAP_PORT));
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

	init_default_apn_list();
	init_default_COAP_parameters();

	// We will have to select_active_apn() after the rest of the hardware has been initialized.
	// At dawn, look to main()
}
