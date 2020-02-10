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
int write_firmware_block(uint8_t *data, u16_t data_len, bool last_block, size_t total_size)
{
    static u8_t percent_downloaded;
    static u32_t bytes_downloaded;
    u8_t downloaded;
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
    if (bytes_downloaded == 0)
    {
        flash_img_init(&dfu_ctx);
        LOG_DBG("Download firmware started, erasing second bank");
        ret = boot_erase_img_bank(FLASH_BANK1_ID);
        if (ret != 0)
        {
            LOG_ERR("Failed to erase flash bank 1");
            goto cleanup;
        }
        LOG_DBG("Finished erasing bank 1");
    }

    bytes_downloaded += data_len;

    /* display a % downloaded, if it's different */
    if (total_size)
    {
        downloaded = bytes_downloaded * 100 / total_size;
    }
    else
    {
        /* Total size is empty when there is only one block */
        downloaded = 100;
    }

    if (downloaded > percent_downloaded)
    {
        percent_downloaded = downloaded;
        if (percent_downloaded % 10 == 0)
        {
            LOG_DBG("Flash %d%%", percent_downloaded);
        }
    }

    ret = flash_img_buffered_write(&dfu_ctx, data, data_len, last_block);
    if (ret < 0)
    {
        LOG_ERR("Failed to write flash block: %d", ret);
        goto cleanup;
    }

    if (!last_block)
    {
        /* Keep going */
        return ret;
    }

    if (total_size && (bytes_downloaded != total_size))
    {
        LOG_ERR("Early last block, downloaded %d bytes but expected %d",
                bytes_downloaded, total_size);
        ret = -EIO;
    }

cleanup:
    bytes_downloaded = 0;
    percent_downloaded = 0;

    return ret;
}
