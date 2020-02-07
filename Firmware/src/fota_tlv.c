#include "fota_tlv.h"

size_t encode_tlv_string(uint8_t *buf, uint8_t id, const uint8_t *str)
{
    size_t ret = 0;
    buf[ret++] = id;
    buf[ret++] = strlen(str);
    for (uint8_t i = 0; i < strlen(str); i++)
    {
        buf[ret++] = str[i];
    }
    return ret;
}