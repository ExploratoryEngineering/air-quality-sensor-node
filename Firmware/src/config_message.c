#include <zephyr.h>
#include <stdio.h>
#include <logging/log.h>
#include <misc/reboot.h>
#include "config_message.h"
#include "config.pb.h"
#include <pb_encode.h>
#include <pb_decode.h>
#include "config.h"

LOG_MODULE_DECLARE(NVS);

int apn_argument_num = 0;

decoded_config_value decoded_values[APN_COMMAND_ARGUMENTS];


/*
*   string_callback is called for each occurence of string values in the incoming stream.
*/
bool apn_string_callback(pb_istream_t *stream, const pb_field_t *field, void **arg)
{
    uint8_t string_buffer[CONFIG_NAME_SIZE] = {0};

    if (stream->bytes_left > CONFIG_NAME_SIZE - 1)
        return false;
    if (!pb_read(stream, string_buffer, stream->bytes_left))
        return false;
    
    decoded_config_value * decoded_value = (decoded_config_value *)(*arg);
    strcpy(decoded_value->string_val, string_buffer);

    return true;
}

/*   
*   Callback for decoding config_Value
*/
bool apn_value_callback(pb_istream_t *stream, const pb_field_t *field, void **arg)
{
	config_Value value = config_Value_init_zero;
	value.stringVal.funcs.decode = &apn_string_callback;

    decoded_config_value * decoded_value = (decoded_config_value *)(*arg);
    value.stringVal.arg = decoded_value;
 	if (!pb_decode(stream, config_Value_fields, &value))
            return false;

    decoded_value->id = value.id;
    decoded_value->int_val = value.int32Val;
   
    if (apn_argument_num < APN_COMMAND_ARGUMENTS)
    {
        decoded_values[apn_argument_num++] = *decoded_value;
        return true;
    }
  
    return false;
}

size_t encode_response(config_response r, uint8_t * buffer, size_t buf_size)
{
    #pragma message("TODO: Fix broken encode_response function.")
    return 0;

    
    size_t message_length = 0;

    config_Response response;
    response.id = r.id;
    response.command = r.command;
    response.responseCode = r.responseCode;

    memset(buffer, 0, buf_size);

    pb_ostream_t stream = pb_ostream_from_buffer(buffer, buf_size);

    bool status = pb_encode(&stream, config_Response_fields, &response);
    message_length = stream.bytes_written;
        
    if (!status)
    {
        LOG_ERR("Encoding of config request response failed:\n");
        return 0;
    }

    return message_length;
}


/*   
*   Decode SET APN LIST config command
*/
config_response decode_set_apn_list_command(uint8_t * buf, int size)
{
	LOG_INF("Decoding SET APN COMMAND");

    config_response response;

    decoded_config_value decoded_value;
    apn_argument_num = 0;

	pb_istream_t stream = pb_istream_from_buffer(buf, size);
	config_Request request = config_Request_init_zero;
	request.values.funcs.decode = &apn_value_callback;
	request.values.arg = &decoded_value;

    response.command = COMMAND_SET_APN_LIST;
	if (!pb_decode(&stream, config_Request_fields, &request))
	{
		LOG_ERR("protobuf decoding failed (request");
        response.responseCode = CONFIG_COMMAND_INVALID_PROOBUF;
		return response;
	}

    response.id = request.id;
    response.responseCode = save_new_apn_config(apn_argument_num);
    return response;
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


config_response decode_config_message(uint8_t * buf, int size)
{
    config_response response;
   	LOG_INF("Decoding incoming message");

    int message_type = get_message_type(buf, size);

    switch(message_type)
    {
        case COMMAND_SET_APN_LIST : return decode_set_apn_list_command(buf, size);
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

    return response;
}
