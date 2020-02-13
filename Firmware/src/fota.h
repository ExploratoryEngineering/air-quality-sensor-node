#pragma once

#include "version.h"
// This is the reported manufacturer reported by the LwM2M client. It is an
// arbitrary string and will be exposed through the Horde API.
#define CLIENT_MANUFACTURER "Exploratory Engineering"

// This is the model number reported by the LwM2M client. It is an arbitrary
// string and will be exposed by the Horde API.
#define CLIENT_MODEL_NUMBER "EE-AQ-2.0"

// This is the serial number reported by the LwM2M client. If you have some
// kind of serial number available you can use that, otherwise the IMEI (the
// ID for the cellular modem) or IMSI (The ID of the SIM in use)
#define CLIENT_SERIAL_NUMBER AQ_NAME

// This is the version of the firmware. This must match the versions set on the
// images uploaded via the Horde API (at https://api.nbiot.engineering/)
#define CLIENT_FIRMWARE_VER AQ_VERSION

int fota_init();

/**
 * @brief Disable FOTA reboots temporarily.
 */
void fota_disable();

/**
 * @brief Enable FOTA reboots
 */
void fota_enable();

typedef struct
{
    char host[25];
    uint32_t port;
    char path[25];
    bool scheduled_update;
} simple_fota_response_t;

int fota_report_version(simple_fota_response_t *resp);

int fota_download_image(simple_fota_response_t *resp);

int fota_encode_simple_report(uint8_t *buffer, size_t *len);
int fota_decode_simple_response(simple_fota_response_t *resp, const uint8_t *buf, size_t len);