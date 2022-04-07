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
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

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
    savedssidNumber = 0;
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
    esp_err_t ret = ESP_FAIL;
    WifiDirectory::HotspotEntry hotspot = {};
    LittleFS wifiPartition1;
    FILE *wifiFile = wifiPartition1.Open("config1.txt", "r");
    if (wifiFile == NULL) // no saved file // create new one
    {
        LOGE(TAG, "No file found..");
        LOGW(TAG, "Creating new file w/config1.txt ..  ");
        wifiFile = wifiPartition1.Open("config1.txt", "w+b");
        if (wifiFile == NULL)
        {
            LOGE(TAG, "File creation failed ... exiting");
            return ESP_FAIL;
        }
        LOG_WIFI_DIRECTORY(TAG, "file created succefully ...no entries over here, adding default entry ");
        hotspot.ssid = "SFR_3DD0";
        hotspot.password = "11112222";
        wifiList.push_back(hotspot);
        fprintf(wifiFile, (const char *)StructToJson(hotspot));
        ret = wifiPartition1.Close();
        wifiCurrentEntryIterator = wifiList.begin();
        savedssidNumber = wifiList.size();
        if (ret == ESP_OK)
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
    char *buffer = (char *)malloc(BUFFER_SIZE);
    while (fgets(buffer, BUFFER_SIZE, wifiFile))
    {
        WifiDirectory::HotspotEntry entry = WifiDirectory::JsonToStruc(buffer);
#if DEBUG_WIFI_DIRECTORY == VERBOS
        isInitialized = true;
        WifiDirectory::PrintEntry(entry);
#endif
        if ((entry.ssid.length()) && (entry.password.length() > 7))
            wifiList.push_back(entry);
    }
    free(buffer);
    ret = wifiPartition1.Close();
    wifiCurrentEntryIterator = wifiList.begin();
    savedssidNumber = wifiList.size();
    if (ret != ESP_OK)
    {
        isInitialized = false;
        return ESP_FAIL;
    }
    isInitialized = true;
    LOG_WIFI_DIRECTORY(TAG, "wifiDirectory is loaded and initialized with %d entries ", wifiList.size());
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
esp_err_t WifiDirectory::CreatePermanentEntry(const HotspotEntry &tempHotspot)
{
    ASSERT_CHARACHTERS_AND_RETURN_ERR(tempHotspot.ssid.c_str())
    ASSERT_CHARACHTERS_AND_RETURN_ERR(tempHotspot.password.c_str())
    if ((tempHotspot.password.length()) < 8) //assert password
    {
        LOGE(TAG, "the entered password %s is not valid for add/update", tempHotspot.password.c_str());
        return ESP_ERR_INVALID_ARG;
    }
    for (auto it : wifiList) //check fo duplicate
    {
        if ((it.ssid == tempHotspot.ssid)) //duplicate found : update its password
        {
            LOGW(TAG, "ssid %s Already exist ... ", tempHotspot.ssid.c_str());
            return ESP_ERR_INVALID_STATE;
        }

    } // no duplicate
    // add new entry in the right priority position
    wifiList.push_back(tempHotspot);
    if (wifiList.size() > LIST_SIZE) // check if list is overflowed? delete lowest priority entry
    {
        LOGW(TAG, " Wifi wifiDirectory list exceeded %d .. deleting ssid: %s", LIST_SIZE, wifiList.back().ssid.c_str());
        wifiList.erase(--wifiList.end());
    }
    ASSERT_INIT_AND_RETURN_ERR(WifiDirectory::isInitialized, TAG)
    LOG_WIFI_DIRECTORY(TAG, " Entry with ssid : %s is added", tempHotspot.ssid.c_str());
#if DEBUG_WIFI_DIRECTORY == VERBOS
    PrintSavedSSID();
#endif
    savedssidNumber = wifiList.size();
    isModified = true;
    Save();
    return ESP_OK;
}

/**
 * @brief wrap up the CreateEntry to force a non-permanent entry add/update
 * 
 * @param tempHotspot entry to be added
 * @return esp_err_t 
 *      ESP_ERR_INVALID_ARG : character/password assertion failed
 *      ESP_ERR_INVALID_STATE : unauthorized opeartion 
 */
esp_err_t WifiDirectory::CreateEntry(const HotspotEntry &tempHotspot)
{
    auto tmp = tempHotspot;
    return CreatePermanentEntry(tmp);
}

/**
 * @brief ModifyEntry can modify password / permanent / priority
 *        will search for existing ssid
 * @param tempHotspot 
 * @return esp_err_t 
 */
esp_err_t WifiDirectory::ModifyEntry(const HotspotEntry &tempHotspot)
{
    ASSERT_INIT_AND_RETURN_ERR(WifiDirectory::isInitialized, TAG)
    ASSERT_CHARACHTERS_AND_RETURN_ERR(tempHotspot.ssid.c_str())
    for (auto it : wifiList) //check fo duplicate
    {
        if ((it.ssid == tempHotspot.ssid) == 0) //duplicate found : update its password
        {
            if ((tempHotspot.password.length()) >= 8)
            {
                it.password = tempHotspot.password;
                LOG_WIFI_DIRECTORY(TAG, " Entry with ssid : %s updated the password..", tempHotspot.ssid.c_str());
            }
            LOG_WIFI_DIRECTORY(TAG, " Entry with ssid %s updated", tempHotspot.ssid.c_str());
            isModified = true;
            return ESP_OK;
        }

    } // not found
    LOGE(TAG, "ssid %s not found for modify ", tempHotspot.ssid.c_str());
    return ESP_ERR_NOT_FOUND;
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
    LittleFS wifiPartition1;
    if (wifiPartition1.IsMounted() == false)
    {
        ASSERT_RET_WITH_MSG_RETURN_RET(ESP_FAIL, "partition  mmounted FILED ... ")
    }
    FILE *wifiFile = wifiPartition1.Open("config1.txt", "w+");
    if (wifiFile == NULL) // no saved file // create new one
    {
        LOGE(TAG, "Fatal error!, file open for saving error , unmounting");
        wifiPartition1.Close();
        return ESP_FAIL;
    }
    LOG_WIFI_DIRECTORY_V(TAG, "FIle Open for write ");
    char *buf = (char *)malloc(BUFFER_SIZE);
    for (auto it = wifiList.begin(); it != wifiList.end(); ++it) // copy all the list to file
    {
        memset(buf, 0, BUFFER_SIZE);
        strlcpy(buf, StructToJson(*it), BUFFER_SIZE);
        fprintf(wifiFile, "%s \n", buf);
    }
    free(buf);
    ret = wifiPartition1.Close();
    if (ret != ESP_OK)
    {
        LOGE(TAG, "Error: Changes may not be saved..");
    }
    savedssidNumber = wifiList.size();
    isModified = false;
    LOG_WIFI_DIRECTORY_V(TAG, "FIle closed and saved , all good");

    return ESP_OK;
}

/**
 * @brief delet only non permanent entry by searching its ssid
 * 
 * @param ssid 
 * @return esp_err_t 
 * 
 */
esp_err_t WifiDirectory::DeleteEntry(const char *ssid)
{
    ASSERT_INIT_AND_RETURN_ERR(WifiDirectory::isInitialized, TAG)
    ASSERT_CHARACHTERS_AND_RETURN_ERR(ssid)
    wifiList.remove_if([&](WifiDirectory::HotspotEntry entry)
                       { return ((entry.ssid == ssid)); });
    if (wifiList.size() < savedssidNumber)
    {
        LOG_WIFI_DIRECTORY(TAG, "ssid %s is removed ", ssid);
        isModified = true;
        savedssidNumber = wifiList.size();
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
 * @brief return next entry in the list 
 * 
 * @return WifiDirectory::HotspotEntry 
 */
const WifiDirectory::HotspotEntry &WifiDirectory::GetNextEntry()
{
    if (!isInitialized)
    {
        LOGE(TAG, " NOT initialized ! returning ");
        static WifiDirectory::HotspotEntry hotspot = {"N/A", "N/A", ""};
        return hotspot;
    }
    if (wifiCurrentEntryIterator == wifiList.end())
    {
        //LOG_WIFI_DIRECTORY_V(TAG, " End of wifi list ... returning to first entry");
        wifiCurrentEntryIterator = wifiList.begin();
    }
    return *(wifiCurrentEntryIterator++);
}

/**
 * @brief print the list of all entries 
 * 
 */
void WifiDirectory::PrintSavedSSID()
{
    ASSERT_INIT_NO_RETURN(WifiDirectory::isInitialized, TAG)
    printf(" list size is : %d \n\r", wifiList.size());
    for (auto it = wifiList.begin(); it != wifiList.end(); it++)
    {
        PrintEntry(*it);
    }
}

/**
 * @brief print info of an entry 
 * 
 * @param entry entry to be printed
 */
void WifiDirectory::PrintEntry(const HotspotEntry &entry)
{
    ASSERT_INIT_NO_RETURN(WifiDirectory::isInitialized, TAG)
    if ((entry.ssid.length() > 1))
        printf("entry: ssid : %s | Pass: %s | bssid: %s \n\r",
               entry.ssid.c_str(), entry.password.c_str(),  entry.bssid.c_str());
}

/**
 * @brief 
 * 
 */
void WifiDirectory::ResetIndex()
{
    ASSERT_INIT_NO_RETURN(WifiDirectory::isInitialized, TAG)
    wifiCurrentEntryIterator = wifiList.begin();
}

/**
 * @brief 
 * 
 */
uint8_t WifiDirectory::GetListNumber()
{
    ASSERT_INIT_RETURN_ZERO(WifiDirectory::isInitialized, TAG)
    return savedssidNumber;
}

/////////////////////////////////Webserver specifi functions ////
/////////////////mainly just wrapping the stuff up //////////////

char *WifiDirectory::StructToJson(const HotspotEntry &entry)
{
    cJSON *entryJson = cJSON_CreateObject();
    AssertNull_RETURN_NULL(entryJson, TAG);
    cJSON_AddStringToObject(entryJson, "ssid", entry.ssid.c_str());
    cJSON_AddStringToObject(entryJson, "password", entry.password.c_str());
    cJSON_AddStringToObject(entryJson, "bssid", entry.bssid.c_str());
    char *charRet = cJSON_PrintUnformatted(entryJson);
    cJSON_Delete(entryJson);
    AssertNull_RETURN_NULL(charRet, TAG);
    return charRet;
}
WifiDirectory::HotspotEntry WifiDirectory::JsonToStruc(char *entrystring)
{
    HotspotEntry entry = {};
    cJSON *entryJson = cJSON_Parse(entrystring);
    if (entryJson == NULL)
    {
        LOGE(TAG, "IMPORT JSON FAILED");
        cJSON_Delete(entryJson);
        return entry;
    }
    char *ssid = cJSON_GetStringValue(cJSON_GetObjectItem(entryJson, "ssid"));
    if (ssid)
        (entry.ssid = (ssid));
    else
    {
        LOGE(TAG, "JSON ENTRY CURRUPTED");
        cJSON_Delete(entryJson);
        return entry;
    }
    char *password = cJSON_GetStringValue(cJSON_GetObjectItem(entryJson, "password"));
    if (password)
        entry.password = password;
    char *bssid = cJSON_GetStringValue(cJSON_GetObjectItem(entryJson, "bssid"));
    if (bssid)
        entry.bssid = bssid;
    cJSON_Delete(entryJson);
    return entry;
}

std::unique_ptr<WifiDirectory> wifiDirectory = nullptr;