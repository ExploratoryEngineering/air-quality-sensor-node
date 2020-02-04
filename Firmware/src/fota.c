#include <zephyr.h>
#include <logging/log.h>

#include <dfu/mcuboot.h>
#include <dfu/flash_img.h>
#include <misc/reboot.h>
#include <net/lwm2m.h>
#include <stdio.h>

#include "fota.h"


LOG_MODULE_REGISTER(fota, LOG_LEVEL_DBG);

#define FLASH_AREA_IMAGE_SECONDARY DT_FLASH_AREA_IMAGE_1_ID
#define FLASH_BANK1_ID DT_FLASH_AREA_IMAGE_1_ID
#define FLASH_BANK_SIZE DT_FLASH_AREA_IMAGE_1_SIZE

static struct k_delayed_work reboot_work;

static void do_reboot(struct k_work *work)
{
	int ret = boot_request_upgrade(BOOT_UPGRADE_TEST);
	if (ret)
	{
		LOG_ERR("boot_request_upgrade: %d", ret);
	}

	sys_reboot(0);
}

static int firmware_update_cb(u16_t obj_inst_id)
{
	LOG_INF("Executing firmware update");

	// Wait a few seconds before rebooting so that the lwm2m client has a chance
	// to acknowledge having received the Update signal.
	k_delayed_work_submit(&reboot_work, K_SECONDS(10));
	return 0;
}

static void *firmware_get_buf(u16_t obj_inst_id, u16_t res_id, u16_t res_inst_id, size_t *data_len)
{
	static u8_t firmware_buf[CONFIG_LWM2M_COAP_BLOCK_SIZE];
	*data_len = sizeof(firmware_buf);
	return firmware_buf;
}

static struct flash_img_context dfu_ctx;

static int firmware_block_received_cb(u16_t obj_inst_id, u16_t res_id, u16_t res_inst_id,
									  u8_t *data, u16_t data_len, bool last_block, size_t total_size)
{
#if defined(CONFIG_FOTA_ERASE_PROGRESSIVELY)
	static int last_offset = DT_FLASH_AREA_IMAGE_1_OFFSET;
#endif
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
#if defined(CONFIG_FOTA_ERASE_PROGRESSIVELY)
		LOG_INF("Download firmware started, erasing progressively.");
		/* reset image data */
		ret = boot_invalidate_slot1();
		if (ret != 0)
		{
			LOG_ERR("Failed to reset image data in bank 1");
			goto cleanup;
		}
#else
		LOG_INF("Download firmware started, erasing second bank");
		ret = boot_erase_img_bank(FLASH_BANK1_ID);
		if (ret != 0)
		{
			LOG_ERR("Failed to erase flash bank 1");
			goto cleanup;
		}
#endif
	}

	bytes_downloaded += data_len;
	LOG_DBG("%d of %d bytes downloaded", bytes_downloaded, total_size);
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

#if defined(CONFIG_FOTA_ERASE_PROGRESSIVELY)
	/* Erase the sector that's going to be written to next */
	while (last_offset <
		   DT_FLASH_AREA_IMAGE_1_OFFSET + dfu_ctx.bytes_written +
			   DT_FLASH_ERASE_BLOCK_SIZE)
	{
		LOG_INF("Erasing sector at offset 0x%x", last_offset);
		flash_write_protection_set(flash_dev, false);
		ret = flash_erase(flash_dev, last_offset,
						  DT_FLASH_ERASE_BLOCK_SIZE);
		flash_write_protection_set(flash_dev, true);
		last_offset += DT_FLASH_ERASE_BLOCK_SIZE;
		if (ret)
		{
			LOG_ERR("Error %d while erasing sector", ret);
			goto cleanup;
		}
	}
#endif

	ret = flash_img_buffered_write(&dfu_ctx, data, data_len, last_block);
	if (ret < 0)
	{
		LOG_ERR("Failed to write flash block");
		goto cleanup;
	}

	if (!last_block)
	{
		/* Keep going */
		return ret;
	}

	if (total_size && (bytes_downloaded != total_size))
	{
		LOG_ERR("Early last block, downloaded %d, expecting %d",
				bytes_downloaded, total_size);
		ret = -EIO;
	}

cleanup:
#if defined(CONFIG_FOTA_ERASE_PROGRESSIVELY)
	last_offset = DT_FLASH_AREA_IMAGE_1_OFFSET;
#endif
	bytes_downloaded = 0;
	percent_downloaded = 0;

	return ret;
}

static int init_lwm2m_resources()
{
	LOG_INF("Firmware version: %s", CLIENT_FIRMWARE_VER);
	LOG_INF("Model number:     %s", CLIENT_MODEL_NUMBER);
	LOG_INF("Serial numbera:   %s", CLIENT_SERIAL_NUMBER);
	LOG_INF("Manufacturer:     %s", CLIENT_MANUFACTURER);

	LOG_INF("This is the new version of the firmware!");

	char *server_url;
	u16_t server_url_len;
	u8_t server_url_flags;
	int ret = lwm2m_engine_get_res_data("0/0/0", (void **)&server_url, &server_url_len, &server_url_flags);
	if (ret)
	{
		LOG_ERR("Error getting LwM2M server URL data: %d", ret);
		return ret;
	}
	snprintk(server_url, server_url_len, "coap://172.16.15.14:5683");

	// Security Mode (3 == NoSec)
	ret = lwm2m_engine_set_u8("0/0/2", 3);
	if (ret)
	{
		LOG_ERR("Error setting LwM2M security mode: %d", ret);
		return ret;
	}

	// Firmware pull uses the buffer set by the Package resource (5/0/0) pre-write callback
	// for passing downloaded firmware chunks to the firmware write callback.
	lwm2m_engine_register_pre_write_callback("5/0/0", firmware_get_buf);
	lwm2m_firmware_set_write_cb(firmware_block_received_cb);

	lwm2m_engine_set_res_data("3/0/0", CLIENT_MANUFACTURER,
							  sizeof(CLIENT_MANUFACTURER),
							  LWM2M_RES_DATA_FLAG_RO);

	lwm2m_engine_set_res_data("3/0/1", CLIENT_MODEL_NUMBER,
							  sizeof(CLIENT_MODEL_NUMBER),
							  LWM2M_RES_DATA_FLAG_RO);

	lwm2m_engine_set_res_data("3/0/2", CLIENT_SERIAL_NUMBER,
							  sizeof(CLIENT_SERIAL_NUMBER),
							  LWM2M_RES_DATA_FLAG_RO);

	lwm2m_engine_set_res_data("3/0/3", CLIENT_FIRMWARE_VER,
							  sizeof(CLIENT_FIRMWARE_VER),
							  LWM2M_RES_DATA_FLAG_RO);

	k_delayed_work_init(&reboot_work, do_reboot);
	lwm2m_firmware_set_update_cb(firmware_update_cb);

	return 0;
}

static int init_image()
{
	// This might need some additional checks. If the update has failed
	// we'll be booting into an confirmed image but this might also be for
	// other reasons. Store the current state somewhere in flash before making
	// assumptions on the state. For now we'll just confirm the image.
	if (!boot_is_img_confirmed())
	{
		int ret = boot_write_img_confirmed();
		if (ret)
		{
			LOG_ERR("confirm image: %d", ret);
			return ret;
		}
		LOG_INF("Firmware update succeeded");
		lwm2m_engine_set_u8("5/0/5", RESULT_SUCCESS);
	}

	return 0;
}

int fota_init()
{
	int ret = init_lwm2m_resources();
	if (ret)
	{
		LOG_ERR("init_lwm2m_resources: %d", ret);
		return ret;
	}

	ret = init_image();
	if (ret)
	{
		LOG_ERR("init_image: %d", ret);
		return ret;
	}

	static struct lwm2m_ctx client;
	lwm2m_rd_client_start(&client, "ee02", NULL);

	return 0;
}
