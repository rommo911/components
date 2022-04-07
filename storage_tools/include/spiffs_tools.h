/*
 * LittleFS.h
 *
 *  Created on: Jun 22, 2020
 *      Author: rami
 */

#ifndef MAIN_LittleFS_H_
#define MAIN_LittleFS_H_
#pragma once

#ifdef ESP_PLATFORM
#include "esp_littlefs.h"
#endif // ESP_PLATFORM

#include "storage_tools.h"
/**
 * @brief constructing an object will mount the file system at the mounting point  CONFIG_LittleFS_DEFAULT_PATH
 *  and default partition 
 * CONFIG_LittleFS_DEFAULT_PARTITION
 */
class LittleFS : public Storage
{
private:
	esp_vfs_littlefs_conf_t configuration;
	esp_err_t format(const char *name) override;
	std::string partitionName;
	static std::timed_mutex lock;

public:
	//mount directly upon definition
	LittleFS(std::string _partitionName = CONFIG_LittleFS_DEFAULT_PARTITION, const std::string &_mountPoint = CONFIG_LittleFS_DEFAULT_PATH) : Storage()
	{
		mountPoint = _mountPoint;
		partitionName = _partitionName;
		isMounted = false;
		lastOpened = NULL;
		Mount();
	};
	//unmount directly after exiting scope
	~LittleFS()
	{
		Unmount();
	}
	//will be called automatically
	esp_err_t Mount();
	esp_err_t Unmount();
	uint64_t GetFreeSpace() override;
	bool IsMounted() override;
};

#endif /* MAIN_LittleFS_H_ */
