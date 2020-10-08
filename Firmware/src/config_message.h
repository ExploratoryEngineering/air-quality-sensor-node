#ifndef _CONFIG_MESSAGE_
#define _CONFIG_MESSAGE_

#include <stdint.h>
#include "config.h"

#define FALLBACK_APN "telenor.iot"
#define FALLBACK_IP "88.99.192.151"

typedef struct _apn_config {
    char apn1[64];
    char apn2[64];
    char apn3[64];
    char apn4[64];
    char coap1[128];
    char coap2[128];
    char coap3[128];
    char coap4[128];
} apn_config;

int decode_config_message(uint8_t * buf, int size);
int encode_ping(char * buffer, int buffer_size, int  * encoded_length);


#endif // _CONFIG_MESSAGE_