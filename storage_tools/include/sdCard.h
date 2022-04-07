/*
 * SD.h
 *
 *  Created on: Jun 22, 2020
 *      Author: rami
 */

#ifndef MAIN_SD_H_
#define MAIN_SD_H_
#pragma once
#include "storage_tools.h"

#ifdef ESP_PLATFORM
#include "driver/sdmmc_host.h"
#include "driver/spi_common.h"
#include "esp_vfs_fat.h"
#endif

class SD : public Storage
{
private:
	esp_vfs_fat_sdmmc_mount_config_t configuration;
	static constexpr esp_vfs_fat_sdmmc_mount_config_t defaultConfig = {
		.format_if_mount_failed = true,
		.max_files = 20,
		.allocation_unit_size = 0,
	};
	sdmmc_card_t *card;
	spi_host_device_t sdHostId;
	sdmmc_host_t host;
	//*HW define pin
	static const gpio_num_t insertedPin = GPIO_NUM_34;
	static bool spiInitialized;
	static bool SdMounted;
	static int Frequency;
	esp_err_t format(const char *name) override;

public:
	SD(const esp_vfs_fat_sdmmc_mount_config_t &conf = defaultConfig, const std::string &_mount_point = std::string("/sdcard"));
	~SD();
	esp_err_t Mount();
	esp_err_t Unmount();
	bool IsMounted() override;
	bool IsInserted() const;
	uint64_t GetFreeSpace() override;
	const int &GetFrequency() const;
	std::string GetSDInfoString()const;
	const sdmmc_card_t *GetSDInfo() const
	{
		return card;
	}
};

#endif /* MAIN_SD_H_ */
