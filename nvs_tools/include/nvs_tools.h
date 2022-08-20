/**
 * @file __nvs.hpp
 * @author Rami
 * @brief
 * @version 0.1
 * @date 2021-01-19
 *
 * @copyright Copyright (c) 2021
 *
 */

#ifndef COMPONENTS_CPP_UTILS_CPPNVS_H_
#define COMPONENTS_CPP_UTILS_CPPNVS_H_
#include "assertion_tools.h"
#include "debug_tools.h"
#include <memory>
#include <nvs.h>
#include <nvs_handle.hpp>
#include <string>
#include <mutex>
#include <string>
#define ASSERT_OPEN                            \
	if (!open)                                 \
	{                                          \
		LOGE(TAG, "Error handle is not ope,"); \
		return ESP_ERR_INVALID_STATE;          \
	}

 /**
  * @brief Provide Non Volatile Storage access.
  */
class NVS
{
	protected:
	static constexpr char TAG[] = "NVS_CPP";

	public:
	//constructo, will set namespace and open mode
	NVS(const std::string& name = "default", nvs_open_mode openMode = NVS_READONLY, const std::string& partition = "nvs");
	NVS(const char* name , nvs_open_mode openMode = NVS_READONLY, const char* partition = "nvs");
	//open the nvs handle
	esp_err_t Init();
	// commit(save changes) and close (deinit)
	esp_err_t close();
	//will call close()
	~NVS();
	//save changes
	esp_err_t commit();
	//erase all the namespace
	esp_err_t erase();
	//erase a key in name space
	esp_err_t erase(const std::string& key);
	//erase a key in name space
	esp_err_t erase(const char* key);
	//get a key (all types but char * or string)
	template <typename T>
	esp_err_t get(const char* key, T& result);
	// workaround for float
	esp_err_t get(const char* key, float& result);
	//set a key (all types but char * nor string)
	template <typename T>
	esp_err_t set(const char* key, const T& data);
	esp_err_t set(const char* key, const float& data);
	//get string key , only to std::string
	esp_err_t getS(const char* key, std::string& result, bool isBlob = false);
	// set a string key using std::string
	esp_err_t setS(const char* key, const std::string& value, bool isBlob = false);
	//set set a string key using char* string lateral
	esp_err_t setS(const char* key, const char* val, bool isBlob = false);
	//get an item size (using key as char *)
	size_t get_size(nvs::ItemType datatype, const char* key);
	//
	bool isOpen()
	{
		return open;
	}

	private:
	//this is to asser init_nvs() function (ESPglobal nvs block)
	static bool isInitialized;
	//changes where made, commit will apply
	bool changed;
	//to indicate the class is constructed but init/!init
	bool open;
	//namespace storage
	std::string m_name;
	std::string m_partitionName;
	nvs_open_mode m_openMode;
	//the main nvs handle ( ESP cxx style )
	std::shared_ptr<nvs::NVSHandle> m_handle;
	//semaphore for local calss
	std::timed_mutex lock;
	//semaphore on instance level
	static std::timed_mutex instancelLock;
	static std::unique_lock<std::timed_mutex> InstanceLock2;
};

/**
 * @brief Retrieve a valu / blob value by key.
 *
 * @param [in] key The key to read from the namespace.
 * @param [out] result The string read from the %NVS storage.
 */
template <typename T>
esp_err_t NVS::get(const char* key, T& result)
{

	ASSERT_INIT_AND_RETURN_ERR(NVS::isInitialized, TAG)
		ASSERT_OPEN
		AssertNull_RETURN_ERR(m_handle, TAG);
	std::lock_guard<std::timed_mutex> guard(lock);
	esp_err_t ret = ESP_FAIL;
	ret = m_handle->get_item(key, result);
	int val = static_cast<int>(result);
	if (ret != ESP_OK)
	{
		LOGE(TAG, "%s Error getting key: %s =%d :: %s", m_name.c_str(), key, val, esp_err_to_name(ret));
		return ret;
	}
	//LOGI(TAG, " %s getting key: %s %d", m_name.c_str(), key, val);
	return ESP_OK;
}

/**
 * @brief Set the value/blob value by key.
 *
 * @param [in] key The key to set from the namespace.
 * @param [in] data The value to set for the key.
 */
template <typename T>
esp_err_t NVS::set(const char* key, const T& data)
{

	ASSERT_INIT_AND_RETURN_ERR(NVS::isInitialized, TAG);
	ASSERT_OPEN;
	AssertNull_RETURN_ERR(m_handle, TAG);
	std::lock_guard<std::timed_mutex> guard(lock);
	esp_err_t ret = m_handle->set_item(key, data);
	changed = true;
	int val = static_cast<int>(data);
	if (ret != ESP_OK)
	{
		LOGE(TAG, " %s Error %s setting key:%s = %d ", m_name.c_str(), esp_err_to_name(ret), key, val);
		return ret;
	}
	//LOGI(TAG, ">> set %s: key: %s=%d", m_name.c_str(), key, val);
	changed = true;
	return ESP_OK;
}

using NvsPointer = std::unique_ptr<NVS>;
#define OPEN_NVS_DEAFULT() std::make_unique<NVS>("default")
#define OPEN_NVS(tag) std::make_unique<NVS>(tag)
#define OPEN_NVS_W(tag) std::make_unique<NVS>(tag, NVS_READWRITE)
#define OPEN_NVS_DEAFULT_W() std::make_unique<NVS>("default", NVS_READWRITE)

#endif /* COMPONENTS_CPP_UTILS_CPPNVS_H_ */