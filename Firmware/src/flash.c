#include <zephyr.h>
#include <logging/log.h>
#include <drivers/flash.h>
#include <misc/reboot.h>
#include <dfu/mcuboot.h>
#include <dfu/flash_img.h>

LOG_MODULE_REGISTER(FLASH, CONFIG_FOTA_LOG_LEVEL);

#define FLASH_BANK0_ID DT_FLASH_AREA_IMAGE_0_ID
#define FLASH_BANK1_ID DT_FLASH_AREA_IMAGE_1_ID
#define FLASH_BANK_SIZE DT_FLASH_AREA_IMAGE_1_SIZE

static struct flash_img_context dfu_ctx;

// This function is more or less an identical copy to the Foundries.io flash write
// code.
int write_firmware_block(const uint8_t *data, const uint16_t data_len, bool first_block, bool last_block, size_t total_size)
{
    int ret = 0;

    if (total_size > FLASH_BANK_SIZE)
    {
        LOG_ERR("Artifact file size too big (%d)", total_size);
        return -EINVAL;
    }

    if (!data_len)
    {
        LOG_ERR("Data len is zero, nothing to write.");
        return -EINVAL;
    }

    /* Erase bank 1 before starting the write process */
    if (first_block)
    {
        flash_img_init(&dfu_ctx);
        LOG_DBG("Download firmware started, erasing second bank");
        ret = boot_erase_img_bank(FLASH_BANK1_ID);
        if (ret != 0)
        {
            LOG_ERR("Failed to erase flash bank 1: %d", ret);
            return ret;
        }
        LOG_DBG("Finished erasing bank 1");
    }

    return flash_img_buffered_write(&dfu_ctx, (uint8_t *)data, data_len, last_block);
}
