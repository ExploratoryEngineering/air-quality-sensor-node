#include <zephyr.h>
#include <logging/log.h>

#include "fota_tlv.h"

LOG_MODULE_REGISTER(FOTA_TLV, CONFIG_FOTA_LOG_LEVEL);

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

inline uint8_t tlv_id(const uint8_t *buf, size_t idx)
{
    return buf[idx];
}

int decode_tlv_string(const uint8_t *buf, size_t *idx, char *str)
{
    int len = (int)buf[(*idx)++];
    int i = 0;
    for (i = 0; i < len; i++)
    {
        str[i] = buf[(*idx)++];
    }
    str[i] = 0;
    return 0;
}

int decode_tlv_uint32(const uint8_t *buf, size_t *idx, uint32_t *val)
{
    size_t len = (size_t)buf[(*idx)++];
    if (len != 4)
    {
        LOG_ERR("uint32 in TLV buffer isn't 4 bytes");
        return -1;
    }
    *val = 0;
    *val += (buf[(*idx)++] << 24);
    *val += (buf[(*idx)++] << 16);
    *val += (buf[(*idx)++] << 8);
    *val += (buf[(*idx)++]);
    return 0;
}

int decode_tlv_bool(const uint8_t *buf, size_t *idx, bool *val)
{
    size_t len = (size_t)buf[(*idx)++];
    if (len != 1)
    {
        LOG_ERR("bool in TLV buffer isn't 1 byte");
        return -1;
    }

    *val = (buf[(*idx)++] == 1);
    return 0;
}