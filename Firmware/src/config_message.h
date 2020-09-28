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

#define CONFIG_COMMAND_INVALID_PROOBUF -1
#define CONFIG_COMMAND_SAVE_FAILED -1

// Since we have an APN table of size 4 (with the last slot marked as "untouchable"), 
// we'll limit this to 3
#define MAX_VALUE_LIST_SIZE 6

#define APN_COMMAND_ARGUMENTS 12

typedef struct _config_value {
    uint32_t  id;
    uint32_t int_val;
    uint8_t string_val[CONFIG_NAME_SIZE];
} decoded_config_value;

typedef struct _config_response {
	uint32_t id;
	uint32_t command;
	uint32_t sequence;
	uint32_t responseCode;
} config_response;



typedef struct _set_apn_list_command {
    decoded_config_value values[MAX_VALUE_LIST_SIZE];
} set_apn_list_command;

config_response decode_config_message(uint8_t * buf, int size);
size_t encode_response(config_response r, uint8_t * buffer, size_t buf_size);



#endif // _CONFIG_MESSAGE_