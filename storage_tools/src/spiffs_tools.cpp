/*
 * spiffs.cpp
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

#include "spiffs_tools.h"
#include "debug_tools.h"
#include "assertion_tools.h"

////////////////////////////////////////////////////////////////
///
/// static variables, constants and configuration assignments
///
/// ////////////////////////////////////////////////////////////
std::timed_mutex LittleFS::lock;

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
esp_err_t LittleFS::Mount()
{
	lock.lock(); // this is semaphore to deny multiple instances of littleFS
	isBusy = true;			  //set static indicator to busy
	esp_err_t ret = ESP_FAIL;
	configuration.base_path = mountPoint.c_str();
	configuration.partition_label = partitionName.c_str();
	configuration.format_if_mount_failed = true;
	configuration.dont_mount = false;
	isMounted = false;
	lastOpened = NULL;
	used = 0;
	total = 0;
	snprintf(TAG, sizeof(TAG) + strlen(configuration.partition_label), "LittleFS %s", configuration.partition_label);
	LOG_STORAGE(TAG, "mounting  partition ' %s  '....", configuration.partition_label);
	ret = esp_vfs_littlefs_register(&configuration);
	if (ret != ESP_OK)
	{
		if (ret == ESP_FAIL)
		{
			LOGE(TAG, "Mount and format Failed ,check partition table...");
			return ret;
		}
		else if (ret == ESP_ERR_NOT_FOUND)
		{
			LOGE(TAG, "Failed to find LittleFS partition");
			isBusy = false;
			return (ret);
		}
		else if (ret == ESP_ERR_INVALID_STATE)
		{
			LOGW(TAG, "Already Mounted...");
		}
		else
		{
			LOGE(TAG, "Failed to mount LittleFS (%s)", esp_err_to_name(ret));
			isBusy = false;
			return (ret);
		}
	}
	else
	{
		isMounted = esp_littlefs_mounted(configuration.partition_label); // succefull mounting?
		if (!isMounted)
		{
			LOGE(TAG, "Mount or format fail after verification ...");
			isBusy = false;
			return ESP_FAIL;
		}
		ret = esp_littlefs_info(configuration.partition_label, &total, &used);
		if (ret != ESP_OK)
		{
			//some kind of error occured (probably partiotion name
			LOGE(TAG, "Failed to get LittleFS partition information (%s)", esp_err_to_name(ret));
			isBusy = false;
			return (ret);
		}
		LOG_STORAGE(TAG, "Partition size: total: %.1f KB, used: %.1f KB , free: %.1f KB", ((double)total / 1024), ((double)used / 1024), (((double)total - (double)used) / 1024));
		LOG_STORAGE_V(TAG, "Active partition is set to :(%s)", configuration.partition_label);
		LOG_STORAGE_V(TAG, "Mount OK");
		return (ESP_OK);
	}
	return ESP_FAIL;
}
/**
 * @brief unmount the partition
 * 
 * @return esp_err_t 
 */
esp_err_t LittleFS::Unmount()
{
	esp_err_t ret = ESP_FAIL;
	if (!esp_littlefs_mounted(configuration.partition_label)) // assert additional mounting indicator
	{
		LOGE(TAG, "File system not mounterd !");
		return (ESP_FAIL);
	}
	if (lastOpened != NULL) // assert opened file
	{
		LOGW(TAG, "a file is openned ..will try to close ");
		Close();
	}
	// its ok , unmount
	LOG_STORAGE(TAG, " unmounting LittleFS....");
	ret = esp_vfs_littlefs_unregister(configuration.partition_label);
	if (ret == ESP_OK)
	{
		if (!IsMounted())
		{
			LOG_STORAGE(TAG, "LittleFS unmounted succefully");
		}
	}
	else
	{
		LOGE(TAG, "Failed to unmount LittleFS partition (%s)", esp_err_to_name(ret));
	}
	isMounted = false;
	isBusy = false;
	lock.unlock();
	return (ret);
}

/**
 * @brief return free space in bytes
 * 
 * @return uint64_t 
 */
uint64_t LittleFS::GetFreeSpace()
{
	ASSERT_TRUE_WITH_MSG_AND_RETRUN_FAIL((isMounted && isBusy), "Failed ,LittleFS is not mounted ! ");
	esp_err_t ret = esp_littlefs_info(configuration.partition_label, &total, &used);
	if (ret != ESP_OK)
	{
		LOGE(TAG, "Failed to get LittleFS partition information (%s)", esp_err_to_name(ret));
		return (0);
	}
	else
	{
		LOG_STORAGE(TAG, "Partition size: %.1f KB, used: %.1f KB , free: %.1f KB", ((double)total / 1024), ((double)used / 1024), (((double)total - (double)used) / 1024));
		return ((uint64_t)(((double)total - (double)used) / 1024));
	}
}

/**
 * @brief returns true if mounted (little_FS api call)
 * 
 * @return true 
 * @return false 
 */
bool LittleFS::IsMounted()
{
	isMounted = esp_littlefs_mounted(configuration.partition_label);
	return isMounted;
}

/**
 * @brief FORMAT, clear obviousely
 * 
 * @param name 
 * @return esp_err_t 
 */
esp_err_t LittleFS::format(const char *name)
{
	esp_err_t ret;
	ret = esp_littlefs_format(name);
	return ret;
}