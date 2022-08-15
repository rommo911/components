/**
 * @file __nvs.cpp
 * @author rami
 * @brief
 * @version 0.2
 * @date 2021-01-19
 *
 * @copyright Copyright (c) 2021
 *
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

#include "nvs_tools.h"
#include <nvs_flash.h>
#include <esp_err.h>
#include <esp_log.h>
#include <stdlib.h>
#include <string>
std::timed_mutex NVS::instancelLock;
const char NVS::TAG[];
bool NVS::isInitialized;

/**
 * @brief Constructor. will init and prepare to acess
 *
 * @param [in] name The namespace to open for access.
 * @param [in] openMode The open mode.  One of NVS_READWRITE (default) or NVS_READONLY.
 */
NVS::NVS(const std::string& name, nvs_open_mode openMode, const std::string& partition)
{
	NVS(name.c_str(), openMode, partition.c_str());
} // NVS

/**
 * @brief Constructor. will init and prepare to acess
 *
 * @param [in] name The namespace to open for access.
 * @param [in] openMode The open mode.  One of NVS_READWRITE (default) or NVS_READONLY.
 */
NVS::NVS(const char* name, nvs_open_mode openMode, const char* partition)
{
	m_name = name;
	m_openMode = openMode;
	m_partitionName = partition;
	isInitialized = false;
	open = false;
	NVS::Init();
} // NVS

/**
 * @brief init the nvs if not initialzed , lock global instance
 *
 *
 * @return esp_err_t
 */
esp_err_t NVS::Init()
{

	const bool res_lock = instancelLock.try_lock_for(std::chrono::seconds(1));
	if (!res_lock)
	{
		LOGE(TAG, "failed to lock instance ! ");
		return ESP_FAIL;
	}
	esp_err_t ret = ESP_OK;
	std::lock_guard<std::timed_mutex> guard(lock);
	if (!isInitialized)
	{
		ret = nvs_flash_init(); /*nvs_flash_init_partition(m_partitionName.c_str());*/ // Initialize flash
		if (ret == ESP_ERR_NVS_NO_FREE_PAGES)
		{
			LOGE(TAG, "nvs_flash us FULLL !! %s", esp_err_to_name(ret));
			instancelLock.unlock();
			return ret;
		}
	}
	if (ret != ESP_OK)
	{
		LOGE(TAG, "nvs_flash_init: %s", esp_err_to_name(ret));
		instancelLock.unlock();
		return ret;
	}
	isInitialized = true;
	m_handle = nvs::open_nvs_handle(m_name.c_str(), m_openMode, &ret);
	if (ret != ESP_OK)
	{
		LOGE(TAG, "nvs_flash handle open: %s %s", m_name.c_str(), esp_err_to_name(ret));
		instancelLock.unlock();
		return ret;
	}
	open = true;
	return ret;
}

/**
 * @brief get string from key as char *
 *
 * @param key
 * @param result std::string to write on
 * @param isBlob //!not implemented yet // TODO
 * @return esp_err_t error code
 */
esp_err_t NVS::getS(const char* key, std::string& result, bool isBlob)
{

	ASSERT_INIT_AND_RETURN_ERR(NVS::isInitialized, TAG)
		ASSERT_OPEN
		AssertNull_RETURN_ERR(m_handle, TAG);
	size_t size = NVS::get_size(nvs::ItemType::SZ, key);
	esp_err_t ret = ESP_ERR_INVALID_SIZE;
	if (size > 0)
	{
		std::lock_guard<std::timed_mutex> guard(lock);
		char* buf = (char*)malloc(size);
		ret = m_handle->get_string(key, buf, size);
		if (ret != ESP_OK)
		{
			LOGE(TAG, "%s Error getting key:%s %s", m_name.c_str(), key, esp_err_to_name(ret));
			free(buf);
			return ret;
		}
		result = std::string(buf, size);
		//LOGV(TAG, " %s getting key:%s=%s", m_name.c_str(), key,result.c_str());
		free(buf);
	}
	return ret;
} // set

/**
 * @brief set string value to a key
 *
 * @param key
 * @param value  std::string
 * @param isBlob //!not implemented yet // TODO
 * @return esp_err_t
 */
esp_err_t NVS::setS(const char* key, const std::string& value, bool isBlob)
{
	return NVS::setS(key, value.c_str(), isBlob);
}

/**
 * @brief store String to nvs
 *
 * @param key
 * @param val char * overload
 * @param isBlob //!not implemented yet // TODO
 * @return esp_err_t
 */
esp_err_t NVS::setS(const char* key, const char* val, bool isBlob)
{

	ASSERT_INIT_AND_RETURN_ERR(NVS::isInitialized, TAG)
		ASSERT_OPEN
		AssertNull_RETURN_ERR(m_handle, TAG);
	esp_err_t ret = ESP_FAIL;
	ret = m_handle->set_string(key, val);
	changed = true;
	if (ret != ESP_OK)
	{
		LOGE(TAG, "%s Error setting key: %s %s", m_name.c_str(), key, esp_err_to_name(ret));
	}
	//LOGI(TAG, ">> set %s: key: %s, string: value=%s",  m_name.c_str(), key, val);
	return ret;
}

esp_err_t NVS::set(const char* key, const float& data)
{
	esp_err_t ret = setS(key, std::to_string(data));
	return ret;
}

/**
 * @brief workarround for float storage : no native
 * 	its by converting the float to string and inverse
 * @param key
 * @param result
 * @return esp_err_t
 */
esp_err_t NVS::get(const char* key, float& result)
{
	std::string buf_float;
	esp_err_t ret = ESP_FAIL;
	ret = getS(key, buf_float);
	try
	{
		result = std::stof(buf_float);
	}
	catch (const std::exception& e)
	{
		ret = ESP_FAIL;
	}
	return ret;
} // get
/**
 * @brief basically commit :  sava namespace modification to flash
 *
 * @return esp_err_t
 */
esp_err_t NVS::close()
{
	ASSERT_INIT_AND_RETURN_ERR(NVS::isInitialized, TAG)
		AssertNull_RETURN_ERR(m_handle, TAG);
	//nvs_stats_t nvs_stats;
	/*::nvs_get_stats(NULL, &nvs_stats);
	LOG_STORAGE(TAG, "Count: UsedEntries = (%d), FreeEntries = (%d), AllEntries = (%d)",
				nvs_stats.used_entries, nvs_stats.free_entries, nvs_stats.total_entries);*/
	esp_err_t ret = ESP_OK;
	ret = commit();
	open = false;
	instancelLock.unlock();
	return ret;
}

/**
 * @brief Desctructor
 * close will assert & save changes & delete
 */
NVS::~NVS()
{
	if (isInitialized && this->open)
	{
		close();
	}
} // ~NVS

/**
 * @brief Commit any work performed in the namespace.
 */
esp_err_t NVS::commit()
{

	ASSERT_INIT_AND_RETURN_ERR(NVS::isInitialized, TAG)
		AssertNull_RETURN_ERR(m_handle, TAG);
	if (open && changed && m_openMode == NVS_READWRITE)
	{
		changed = false;
		esp_err_t ret = m_handle->commit();
		return ret;
	}
	return ESP_OK;
} // commit

/**
 * @brief Erase ALL the keys in the namespace.
 */
esp_err_t NVS::erase()
{

	ASSERT_INIT_AND_RETURN_ERR(NVS::isInitialized, TAG)
		ASSERT_OPEN
		AssertNull_RETURN_ERR(m_handle, TAG);
	std::lock_guard<std::timed_mutex> guard(lock);
	changed = true;
	esp_err_t ret = m_handle->erase_all();
	return ret;
} // erase

/**
 * @brief Erase a specific key in the namespace.
 *
 * @param [in] key The key to erase from the namespace.
 */
esp_err_t NVS::erase(const std::string& key)
{
	return erase(key.c_str());
} // erase

/**
 * @brief Erase a specific key in the namespace.
 *
 * @param [in] key The key to erase from the namespace.
 */
esp_err_t NVS::erase(const char* key)
{

	ASSERT_INIT_AND_RETURN_ERR(NVS::isInitialized, TAG)
		ASSERT_OPEN
		AssertNull_RETURN_ERR(m_handle, TAG);
	std::lock_guard<std::timed_mutex> guard(lock);
	changed = true;
	esp_err_t ret = m_handle->erase_item(key);
	return ret;
} // erase

size_t NVS::get_size(nvs::ItemType datatype, const char* key)
{

	ASSERT_INIT_AND_RETURN_ERR(NVS::isInitialized, TAG)
		size_t s = 0;
	ASSERT_OPEN
		std::lock_guard<std::timed_mutex> guard(lock);
	esp_err_t ret = m_handle->get_item_size(datatype, key, s);
	if (ret != ESP_OK)
	{
		LOGE(TAG, "%s Error getting key size : %s ! %s", m_name.c_str(), key, esp_err_to_name(ret));
		return 0;
	}
	return s;
}
