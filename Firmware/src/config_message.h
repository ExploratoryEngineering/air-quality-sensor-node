#ifndef _CONFIG_MESSAGE_
#define _CONFIG_MESSAGE_

#include <stdint.h>
#include "config.h"

// Best keep these defines in sync with the config app. Just sayin'
#define COMMAND_STATUS           1
#define COMMAND_REBOOT           2
#define COMMAND_SET_APN_LIST     3
#define COMMAND_GET_APN_LIST     4
#define COMMAND_ECHO            20

// Since we have an APN table of size 4 (with the last slot marked as "untouchable"), 
// we'll limit this to 3
#define APN_LIST_SIZE 3

typedef struct _set_apn_list_command {
    uint32_t id;
    uint32_t apn_count;
    uint8_t apn[APN_LIST_SIZE][APN_NAME_SIZE];
} set_apn_list_command;

void decode_config_message(uint8_t * buf, int size);


#endif // _CONFIG_MESSAGE_