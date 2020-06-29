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

// Server configuration. The reporting endpoint is at coap://172.16.32.1/u and
// the response points to the firmware download (usually coap://172.16.32.1/fw
// but don't quote me on that)
#define FOTA_COAP_SERVER "172.16.32.1"
#define FOTA_COAP_PORT 5683
#define FOTA_COAP_REPORT_PATH "u"

/**
 * @brief Initialize FOTA
 */
bool fota_init();

/**
 * @brief Run a FOTA upgrade
 */
bool fota_run();

/**
 * @brief Wait for response on a socket fd
 */
bool wait_for_response(int sock);