/**
 * @file wifi_directory.cpp
 * @author Rami (you@domain.com)
 * @brief
 * @version 0.1
 * @date 2020-07-27
 *
 * @copyright Copyright (c) 2020
 *
 */

#include "debug_tools.h"
#if (DEBUG_WIFI_DIRECTORY == VERBOS)
#define LOG_WIFI_DIRECTORY LOGI
#define LOG_WIFI_DIRECTORY_V LOGI
#elif (DEBUG_WIFI_DIRECTORY == INFO)
#define LOG_WIFI_DIRECTORY LOGI
#define LOG_WIFI_DIRECTORY_V LOGD
#else
#define LOG_WIFI_DIRECTORY LOGD
#define LOG_WIFI_DIRECTORY_V LOGV
#endif

#include "assertion_tools.h"
#include "wifi_directory.h"
#include "cJSON.h"
#include "esp_log.h"
#include <LittleFS.h>
#include "FS.h"

 /**
  * @brief  assert set of characters allowed
  *
  */
#define ASSERT_CHARACHTERS_AND_RETURN_ERR(str)                                 \
    {                                                                          \
        size_t n;                                                              \
        if (str[n = strspn(str, check)] != '\0' || !(strlen(str)))             \
        {                                                                      \
            LOGE(TAG, "string %s has invalid characters.", str);               \
            LOGE(TAG, "Invalid character is %c at position %d \n", str[n], n); \
            return ESP_ERR_INVALID_ARG;                                        \
        }                                                                      \
    }

  ////////////////////////////////////////////////////////////////
  ///
  /// static variables, constants and configuration assignments
  ///
  /// ////////////////////////////////////////////////////////////

const int WifiDirectory::LIST_SIZE;
const char WifiDirectory::TAG[];
const char WifiDirectory::check[];

/**
 * @brief Destroy the Wifi Directoty:: Wifi Directoty object
 *  free memory occupied by the list
 */
WifiDirectory::~WifiDirectory()
{
    wifiList.clear();
}

/**
 * @brief Construct a new Wifi Directoty:: Wifi Directoty object
 *
 *
 */
WifiDirectory::WifiDirectory()
{
    isInitialized = false;
}

/**
 * @brief
 *          initilize spiffs to zero
 *          read spiffs partition and get ssid list
 *          if no file is present, file will be created and one default entry will be saved
 * @return esp_err_t
 */
esp_err_t WifiDirectory::Init()
{
    esp_err_t ret = ESP_OK;
    WifiDirectory::HotspotEntry hotspot = {};
    if (!LittleFS.begin(true)) {
        return ESP_FAIL;
    }
    File wifiFile = LittleFS.open("/config1.txt", "r");
    if (!wifiFile) // no saved file // create new one
    {
        LOGW(TAG, "No file found.. Creating new file w/config1.txt ..  ");
        wifiFile = LittleFS.open("/config1.txt", "w+b");
        if (!wifiFile)
        {
            LOGE(TAG, "File creation failed ... exiting");
            return ESP_FAIL;
        }
        LOG_WIFI_DIRECTORY(TAG, "file created succefully ...no entries over here, adding default entry ");
        hotspot.ssid = "SFR_3DD0";
        hotspot.password = "11112222";
        wifiList.push_back(hotspot);
        wifiFile.println((const char*)StructToJson(hotspot));
        int ok = wifiFile.close();
        if (ret == ESP_OK && ok == 0)
        {
            isInitialized = true;
            return ESP_OK;
        }
        else
        {
            LOGE(TAG, "something wen wrong when writing to file ");
            return ESP_FAIL;
        }
    }
    // file is present : read content and store them in list
    while (wifiFile.available())
    {
        String l_line = wifiFile.readStringUntil('\r');
        WifiDirectory::HotspotEntry entry = WifiDirectory::JsonToStruc(l_line.c_str());
        if ((entry.ssid.length()) && (entry.password.length() > 7))
            wifiList.push_back(entry);
    }
    if (ret != ESP_OK)
    {
        isInitialized = false;
        return ESP_FAIL;
    }
    isInitialized = true;
    LOG_WIFI_DIRECTORY(TAG, "wifiDirectory is loaded and initialized with %d entries ", wifiList.size());
    LittleFS.end();
    return ESP_OK;
}

/**
 * @brief add a permanent entry to the list and save it
 *          will assert character and duplication
 *          if duplication occures, password will be updated
 *
 * @param tempHotspot entry to be added
 * @return esp_err_t
 *      ESP_ERR_INVALID_ARG : character/password assertion failed
 *      ESP_ERR_INVALID_STATE : unauthorized opeartion
 */
esp_err_t WifiDirectory::CreateEntry(const HotspotEntry& tempHotspot)
{
    ASSERT_INIT_AND_RETURN_ERR(WifiDirectory::isInitialized, TAG)
        ASSERT_CHARACHTERS_AND_RETURN_ERR(tempHotspot.ssid.c_str())
        ASSERT_CHARACHTERS_AND_RETURN_ERR(tempHotspot.password.c_str())
        if ((tempHotspot.password.length()) < 8) //assert password
        {
            LOGE(TAG, "the entered password %s is not valid for add/update", tempHotspot.password.c_str());
            return ESP_ERR_INVALID_ARG;
        }
    for (auto& entry : wifiList) //check fo duplicate
    {
        if ((entry.ssid == tempHotspot.ssid)) //duplicate found : update its password
        {
            if (entry.password != tempHotspot.password)
            {
                entry.password = tempHotspot.password;
                LOG_WIFI_DIRECTORY(TAG, "duplicate found : updating password for %s", entry.ssid.c_str());
                goto end;
            }
            else
            {
                LOGW(TAG, "ssid %s Already exist ... ", tempHotspot.ssid.c_str());
                return ESP_ERR_INVALID_STATE;
            }
        }

    } // no duplicate
    // add new entry in the right priority position
    wifiList.push_back(tempHotspot);
    if (wifiList.size() > LIST_SIZE) // check if list is overflowed? delete lowest priority entry
    {
        LOGW(TAG, " Wifi wifiDirectory list exceeded %d .. deleting ssid: %s", LIST_SIZE, wifiList.back().ssid.c_str());
        wifiList.erase(--wifiList.end());
    }
    LOG_WIFI_DIRECTORY(TAG, " Entry with ssid : %s is added", tempHotspot.ssid.c_str());
#if DEBUG_WIFI_DIRECTORY == VERBOS
    PrintSavedSSID();
#endif
end:
    isModified = true;
    Save();
    return ESP_OK;
}


/**
 * @brief push the list to the config.txt file
 *        note the it will re-write the file.
 *
 * @return esp_err_t
 */
esp_err_t WifiDirectory::Save()
{
    if (!isModified)
    {
        return ESP_OK;
    }
    esp_err_t ret;
    ASSERT_INIT_AND_RETURN_ERR(WifiDirectory::isInitialized, TAG)
        LOG_WIFI_DIRECTORY(TAG, "saving wifi list ... ");
    if (!LittleFS.begin(true)) {
        return ESP_FAIL;
    }
    File wifiFile = LittleFS.open("/config1.txt", "w+");
    if (!wifiFile) // no saved file // create new one
    {
        LOGE(TAG, "Fatal error!, file open for saving error , unmounting");
        LittleFS.end();
        return ESP_FAIL;
    }
    LOG_WIFI_DIRECTORY_V(TAG, "FIle Open for write ");
    for (auto it = wifiList.begin(); it != wifiList.end(); ++it) // copy all the list to file
    {
        wifiFile.println(StructToJson(*it));
    }
    ret = wifiFile.close() == 0 ? ESP_OK : ESP_FAIL;
    if (ret != ESP_OK)
    {
        LOGE(TAG, "Error: Changes may not be saved..");
    }
    isModified = false;
    LOG_WIFI_DIRECTORY_V(TAG, "FIle closed and saved , all good");
    LittleFS.end();
    return ESP_OK;
}

/**
 * @brief delet only non permanent entry by searching its ssid
 *
 * @param ssid
 * @return esp_err_t
 *
 */
esp_err_t WifiDirectory::DeleteEntry(const char* ssid)
{
    ASSERT_INIT_AND_RETURN_ERR(WifiDirectory::isInitialized, TAG);
    wifiList.remove_if([&](WifiDirectory::HotspotEntry entry)
        {
            if (entry.ssid == ssid)
            {
                isModified = true;
                LOG_WIFI_DIRECTORY(TAG, "ssid %s is removed ", ssid);
                return true;
            }
            return false;
        }
    );
    if (isModified == true)
    {
        Save();
        return ESP_OK;
    }
    else
    {
        LOGE(TAG, "ssid %s not found for remove ", ssid);
        return ESP_ERR_NOT_FOUND;
    }
}

esp_err_t WifiDirectory::ResetDirectory()
{
    wifiList.clear();
    return Save();
}


/**
 * @brief print the list of all entries
 *
 */
void WifiDirectory::PrintSavedSSID()
{
    ASSERT_INIT_NO_RETURN(WifiDirectory::isInitialized, TAG)
        printf(" list size is : %d \n\r", wifiList.size());
    for (const auto& entry : wifiList)
    {
        PrintEntry(entry);
    }
}

/**
 * @brief print info of an entry
 *
 * @param entry entry to be printed
 */
void WifiDirectory::PrintEntry(const HotspotEntry& entry)
{
    ASSERT_INIT_NO_RETURN(WifiDirectory::isInitialized, TAG)
        if ((entry.ssid.length() > 1))
            printf("entry: ssid : %s | Pass: %s | bssid: %s \n\r",
                entry.ssid.c_str(), entry.password.c_str(), entry.bssid.c_str());
}


/**
 * @brief
 *
 */
uint8_t WifiDirectory::GetListNumber()
{
    ASSERT_INIT_RETURN_ZERO(WifiDirectory::isInitialized, TAG)
        return wifiList.size();
}

/////////////////////////////////Webserver specifi functions ////
/////////////////mainly just wrapping the stuff up //////////////

char* WifiDirectory::StructToJson(const HotspotEntry& entry)
{
    cJSON* entryJson = cJSON_CreateObject();
    AssertNull_RETURN_NULL(entryJson, TAG);
    cJSON_AddStringToObject(entryJson, "ssid", entry.ssid.c_str());
    cJSON_AddStringToObject(entryJson, "password", entry.password.c_str());
    cJSON_AddStringToObject(entryJson, "bssid", entry.bssid.c_str());
    char* charRet = cJSON_PrintUnformatted(entryJson);
    cJSON_Delete(entryJson);
    AssertNull_RETURN_NULL(charRet, TAG);
    return charRet;
}
WifiDirectory::HotspotEntry WifiDirectory::JsonToStruc(const char* entrystring)
{
    HotspotEntry entry = {};
    cJSON* entryJson = cJSON_Parse(entrystring);
    if (entryJson == NULL)
    {
        LOGE(TAG, "IMPORT JSON FAILED");
        cJSON_Delete(entryJson);
        return entry;
    }
    char* ssid = cJSON_GetStringValue(cJSON_GetObjectItem(entryJson, "ssid"));
    if (ssid)
        (entry.ssid = (ssid));
    else
    {
        LOGE(TAG, "JSON ENTRY CURRUPTED");
        cJSON_Delete(entryJson);
        return entry;
    }
    char* password = cJSON_GetStringValue(cJSON_GetObjectItem(entryJson, "password"));
    if (password)
        entry.password = password;
    char* bssid = cJSON_GetStringValue(cJSON_GetObjectItem(entryJson, "bssid"));
    if (bssid)
        entry.bssid = bssid;
    cJSON_Delete(entryJson);
    return entry;
}

std::unique_ptr<WifiDirectory> wifiDirectory = nullptr;