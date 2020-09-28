#include <zephyr.h>
#include <stdio.h>
#include <logging/log.h>

#include "config_message.h"
#include "config.pb.h"
#include <pb_encode.h>
#include <pb_decode.h>

LOG_MODULE_DECLARE(NVS);

int string_index = 0;
uint8_t string_buffer[APN_LIST_SIZE][APN_NAME_SIZE] = {0};

/*
*   string_callback is called for each occurence of string values in the incoming stream.
*/
bool apn_string_callback(pb_istream_t *stream, const pb_field_t *field, void **arg)
{
    if (stream->bytes_left > APN_NAME_SIZE - 1)
        return false;
    if (!pb_read(stream, &string_buffer[string_index], stream->bytes_left))
        return false;
    
    LOG_INF("Read : %s", log_strdup(string_buffer[string_index]));
  	string_index++;
    return true;
}

/*   NOTE: We're only expecting a list of maximum 3 strings at this time. This will have 
*         to be refactored / extended for other message types
*/
bool apn_value_callback(pb_istream_t *stream, const pb_field_t *field, void **arg)
{
	config_Value value = config_Value_init_zero;
	value.stringVal.funcs.decode = &apn_string_callback;
    // value.stringVal.arg = arg;
 	if (!pb_decode(stream, config_Value_fields, &value))
            return false;
    return true;
}

/*
*   get_message_type is a helper function for identifying message type.
*/
int get_message_type(uint8_t * buf, int size)
{
	LOG_INF("Checking incoming message type");

    pb_istream_t stream = pb_istream_from_buffer(buf, size);
	config_Request request = config_Request_init_zero;

	if (!pb_decode(&stream, config_Request_fields, &request))
	{
		LOG_ERR("protobuf decoding failed (request");
		return -1;
	}
    return request.command;
}

void decode_set_apn_list_command(uint8_t * buf, int size)
{
	LOG_INF("Decoding SET APN COMMAND");

    set_apn_list_command apn;
    string_index = 0;

    memset(&string_buffer, 0, APN_LIST_SIZE*APN_NAME_SIZE);

	pb_istream_t stream = pb_istream_from_buffer(buf, size);
	config_Request request = config_Request_init_zero;
	request.values.funcs.decode = &apn_value_callback;
    // TODO: Pass the correct type as arg and populate while reading the stream
	//request.values.arg = &apn;

	if (!pb_decode(&stream, config_Request_fields, &request))
	{
		LOG_ERR("protobuf decoding failed (request");
		return;
	}

    apn.apn_count = string_index;
    for (int i=0; i<string_index; i++)
    {
        strcpy(apn.apn[i], string_buffer[i]);
        LOG_INF("NEW APN %d : %s", i, log_strdup(apn.apn[i]));
    }

    save_new_apn_config(apn);
	// LOG_INF("Command ID: %d", request.command);

}

void decode_config_message(uint8_t * buf, int size)
{
   	LOG_INF("Decoding incoming message");

    int message_type = get_message_type(buf, size);

    switch(message_type)
    {
        case COMMAND_SET_APN_LIST : decode_set_apn_list_command(buf, size);
                                    break;
        case COMMAND_REBOOT : LOG_INF("Rebooting...");
                                k_sleep(1000);
                                sys_reboot(0);
                                break;
        case COMMAND_STATUS :
        case COMMAND_GET_APN_LIST :
        case COMMAND_ECHO         :
        default:
		       LOG_INF("Unrecognized message type %d. Ignoring request", message_type);
		        break;
    }
}
