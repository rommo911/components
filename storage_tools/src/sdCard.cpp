/*
 * __sd.cpp
 *
 *  Created on: Jun 22, 2020
 *      Author: rami
 */

#include "debug_tools.h"

#if (DEBUG_STORAGE == VERBOS)
#define LOG_STORAGE ESP_LOGI
#define LOG_STORAGE_V ESP_LOGI
#elif (DEBUG_STORAGE == INFO)
#define LOG_STORAGE ESP_LOGI
#define LOG_STORAGE_V ESP_LOGD
#else
#define LOG_STORAGE ESP_LOGD
#define LOG_STORAGE_V ESP_LOGV
#endif

#include "assertion_tools.h"
#include "debug_tools.h"
#include "sdCard.h"
#include "driver/sdspi_host.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sdmmc_cmd.h"
#include "vfs_fat_internal.h"
#include <memory>
#include <sstream>
#include "string.h"
#define PIN_NUM_MISO GPIO_NUM_33 //SDI
#define PIN_NUM_MOSI GPIO_NUM_26 //SDO
#define PIN_NUM_CLK GPIO_NUM_32
#define PIN_NUM_CS GPIO_NUM_5
#define SPI_DMA_CHAN 1

////////////////////////////////////////////////////////////////
///
/// static variables, constants and configuration assignments
///
///////////////////////////////////////////////////////////////
const esp_vfs_fat_sdmmc_mount_config_t SD::defaultConfig;
bool SD::spiInitialized = false;
bool SD::SdMounted;
int SD::Frequency = CONFIG_SD_CARD_FREQ;
bool SD::IsMounted()
{
	return isMounted && SdMounted;
}

SD::SD(const esp_vfs_fat_sdmmc_mount_config_t &conf, const std::string &_mount_point) : Storage()
{
	isMounted = false;
	lastOpened = NULL;
	mountPoint = _mount_point;
	configuration = conf;
	if (/*IsInserted()*/ 1)
	{
		if (!IsMounted())
		{
			Mount();
		}
	};
}
SD::~SD()
{
	LOG_STORAGE_V(TAG, "destructor");
	/*if (IsMounted())
		Unmount();*/
}

/**
 * @fn esp_err_t Mount(esp_vfs_spiffs_conf_t*)
 * @brief initialize and mount spiffs. only ony instance will be mounted at a time.
 * 		  to use multiple instances , close active file, unmount a partition using Unmout()
 * 		  before mounting other instance ( partition)
 *
 * @param conf pointer to partition confuguration to be mounted : most important is
 * 		  partition label : should be same as in the parition csv file
 *
 * @return esp_err_t  FAIL/OK
 */
esp_err_t SD::Mount()
{
	if (isMounted && SdMounted)
		return ESP_OK;
	isBusy = true;
	esp_err_t ret = ESP_OK;
	isMounted = false;
	SdMounted = false;
	lastOpened = NULL;
	used = 0;
	total = 0;
	fsLock.lock();
	// copy mount point to local
	snprintf(TAG, sizeof(TAG) + strlen(mountPoint.c_str()), "SD %s", mountPoint.c_str()); // make a tag
	LOG_STORAGE_V(TAG, "mounting  card ' %s  '....", mountPoint.c_str());
	sdmmc_host_t _host = SDSPI_HOST_DEFAULT(); //create a default sd spi hot and copy to local class storage
	host = _host;
	host.max_freq_khz = Frequency;
	sdHostId = HSPI_HOST;
	spi_bus_config_t bus_cfg = {
		.mosi_io_num = PIN_NUM_MOSI,
		.miso_io_num = PIN_NUM_MISO,
		.sclk_io_num = PIN_NUM_CLK,
		.quadwp_io_num = -1,
		.quadhd_io_num = -1,
		.max_transfer_sz = 0,
		.flags = 0,
		.intr_flags = 0,
	};
	if (!spiInitialized)
	{
		ret = spi_bus_initialize(sdHostId, &bus_cfg, SPI_DMA_CHAN); // start spi bus
	}
	if (ret != ESP_OK)
	{
		LOGE(TAG, "Failed to initialize bus.");
		spi_bus_free(sdHostId);
		spiInitialized = false;
		isBusy = false;
		fsLock.unlock();
		return ESP_FAIL;
	}
	else
	{
		spiInitialized = true;
	}
	sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
	slot_config.gpio_cs = PIN_NUM_CS;
	slot_config.host_id = sdHostId; //set hots (vspi or Hspi)
	ret = esp_vfs_fat_sdspi_mount(mountPoint.c_str(), &host, &slot_config, &configuration, &card);
	if (ret != ESP_OK)
	{
		//spi_bus_free(sdHostId); //release spi host after mount is failed
		if (ret == ESP_FAIL)
		{
			LOGE(TAG, "Failed to mount filesystem. if you want the card to be formatted");
		}
		else
		{
			LOGE(TAG, "Failed to initialize the card (%s)", esp_err_to_name(ret));
			if (ret == ESP_ERR_INVALID_CRC)
				Frequency = Frequency - (Frequency / 4);
		}
		isBusy = false;
		fsLock.unlock();
		return ESP_FAIL;
	}
	//sdmmc_card_print_info(stdout, card);
	isMounted = true;
	SdMounted = true;
	LOG_STORAGE(TAG, "Card is mount ' %s  '....", mountPoint.c_str());
	return ESP_OK;
}

/**
 * @brief unmount the partition
 * 
 * @return esp_err_t 
 */
esp_err_t SD::Unmount()
{
	if (!IsMounted()) // assert additional mounting indicator
	{
		return (ESP_OK);
	}
	// its ok , unmount
	LOG_STORAGE_V(TAG, " unmounting SdCard....");
	esp_err_t ret = esp_vfs_fat_sdcard_unmount(mountPoint.c_str(), card);
	if (ret == ESP_OK)
	{
		isMounted = false;
		isBusy = false;
		SdMounted = false;
		LOG_STORAGE(TAG, "SdCard unmounted succefully");
	}
	else
	{
		LOGE(TAG, "Failed to unmount SdCard partition (%s)", esp_err_to_name(ret));
	}
	fsLock.unlock();
	return (ret);
}

std::string SD::GetSDInfoString() const
{
	bool print_csd;
	std::stringstream outputStream;
	outputStream << "Name: " << card->cid.name;
	if (card->is_sdio)
	{
		outputStream << "SDIO";
		print_csd = true;
	}
	else if (card->is_mmc)
	{
		outputStream << "MMC";
		print_csd = true;
	}
	else
	{
		if ((card->ocr & (1 << 30)))
			outputStream << "SDHC/SDXC";
		else
			outputStream << "SDSC";
	}
	if (card->max_freq_khz < 1000)
	{
		outputStream << "Speed:" << card->max_freq_khz << "kHz\n";
	}
	else
	{
		outputStream << "Speed:" << card->max_freq_khz / 1000 << "MHz\n";
	}
	outputStream << "Size:" << ((((uint64_t)card->csd.capacity) * (card->csd.sector_size)) / (1024 * 1024)) << "MB\n";

	if (print_csd)
	{
		outputStream << "capacity=" << card->csd.capacity << "read_bl_len=" << card->csd.read_block_len << "\n";
	}
	return outputStream.str();
}

/**
 * @brief return free space in bytes
 * 
 * @return uint64_t 
 */
uint64_t SD::GetFreeSpace()
{
	if (isMounted != true)
	{
		ESP_LOGE(TAG, "Failed ,SdCard is not mounted ! ");
		return ESP_FAIL;
	}
	DWORD fre_clust, fre_sect;
	FATFS *fs;
	auto res = f_getfree("/sdcard/", &fre_clust, &fs);
	if (res)
	{
		return 0; //= 0
	}
	fre_sect = fre_clust * fs->csize;
	uint64_t free_bytes = (uint64_t)fre_sect * FF_SS_SDCARD;
	/* Print the free space in bytes */
	return free_bytes;
}

/**
 * @brief 
 * 
 * @return true 
 * @return false 
 */
bool SD::IsInserted() const
{
	gpio_set_direction(insertedPin, GPIO_MODE_INPUT);
	bool ret = (bool)gpio_get_level(insertedPin);
	return ret;
}

const int &SD::GetFrequency() const
{
	return Frequency;
}

static esp_err_t partition_card(const esp_vfs_fat_mount_config_t *mount_config,
								const char *drv, sdmmc_card_t *card, BYTE pdrv)
{
	FRESULT res = FR_OK;
	esp_err_t err;
	const size_t workbuf_size = 4096;
	void *workbuf = NULL;
	ESP_LOGW("TAG", "partitioning card");

	workbuf = ff_memalloc(workbuf_size);
	if (workbuf == NULL)
	{
		return ESP_ERR_NO_MEM;
	}

	DWORD plist[] = {100, 0, 0, 0};
	res = f_fdisk(pdrv, plist, workbuf);
	if (res != FR_OK)
	{
		err = ESP_FAIL;
		ESP_LOGD("TAG", "f_fdisk failed (%d)", res);
		free(workbuf);
		return err;
	}
	size_t alloc_unit_size = esp_vfs_fat_get_allocation_unit_size(
		card->csd.sector_size,
		mount_config->allocation_unit_size);
	ESP_LOGW("TAG", "formatting card, allocation unit size=%d", alloc_unit_size);
	res = f_mkfs(drv, FM_ANY, alloc_unit_size, workbuf, workbuf_size);
	if (res != FR_OK)
	{
		err = ESP_FAIL;
		ESP_LOGD("TAG", "f_mkfs failed (%d)", res);
		goto fail;
	}

	free(workbuf);
	return ESP_OK;
fail:
	free(workbuf);
	return err;
}

/**
 * @brief FORMAT, clear obviousely
 * 
 * @param name 
 * @return esp_err_t 
 */
esp_err_t SD::format(const char *name) //TODO
{
	esp_err_t ret = ESP_FAIL;
	auto FF_DRV_NOT_USED = 0xFF;

	BYTE pdrv = FF_DRV_NOT_USED;
	ESP_LOGD(TAG, "using pdrv=%i", pdrv);
	char drv[3] = {(char)('0' + pdrv), ':', 0};
	ret = partition_card(&configuration, drv, card, pdrv);
	return ret;
}