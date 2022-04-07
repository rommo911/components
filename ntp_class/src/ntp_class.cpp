/**
 * @file __ntp.cpp
 * @author rami (rami@domalys.com)
 * @brief 
 * @version 0.3
 * @date 2020-09-11
 * 
 * @copyright Copyright (c) 2021
 * 
 */

#include "debug_tools.h"

#if (DEBUG_NTP == VERBOS)
#define LOG_NTP LOGI
#define LOG_NTP_V LOGI
#elif (DEBUG_NTP == INFO)
#define LOG_NTP LOGI
#define LOG_NTP_V LOGD
#else
#define LOG_NTP LOGD
#define LOG_NTP_V LOGV
#endif

#include "ntp_class.h"
#include "nvs_tools.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_sntp.h"
#include "esp_system.h"
#include "sdkconfig.h"
#include "string.h"
#include <sys/time.h>
#include <time.h>
bool Ntp::isSynchronized = false;
bool Ntp::busy = false;
unsigned long Ntp::SyncTimeStamb = 0;

Ntp::Ntp() : Config(TAG)
{
}

/**
 * @fn void ntp_obtainTime(void),
 * @brief start sntp service & update 'now' variable with current time 
 * no need to run again to get time, insted get the time from internal clock afterwards
 * no return, it will set the internal clock.
 */
esp_err_t Ntp::SetAndSyncTime(const char *server)
{
    if (!isSynchronized && !busy)
    {
        InitThisConfig();
        busy = true;
        if (server == NULL)
            return ESP_ERR_INVALID_ARG; // assert
        LOG_NTP(TAG, "Initializing SNTP");
        sntp_setoperatingmode(SNTP_OPMODE_POLL);
        sntp_setservername(0, server);
        sntp_set_time_sync_notification_cb(TimeSyncNotificationCallback);
        sntp_set_sync_mode(SNTP_SYNC_MODE_IMMED);
        sntp_set_sync_interval(CONFIG_LWIP_SNTP_UPDATE_DELAY);
        sntp_setservername(1, "0.nl.pool.ntp.org");
        sntp_setservername(2, "1.nl.pool.ntp.org");
        sntp_init();
    }
    return ESP_OK;
}

/**
 * @fn void ntp_time_sync_notification_cb
 * @brief sync event callback, just notify the system and maybe for futher stuff
 *
 */
void Ntp::TimeSyncNotificationCallback(struct timeval *tv)
{
    if (isSynchronized == false)
    {
        isSynchronized = true;
        time_t _now = 0;
        time(&_now);
        SyncTimeStamb = _now;
    }
    busy = false;
    LOG_NTP(TAG, "time synchronization event");
    isSynchronized = true;
}

/**
 * @brief get the time from the internal clock 
 * (internal clock should be updated at least once with SetAndSyncTime
 *  and then no need to ask NTP class for timestamb)
 * 
 * @return struct tm 
 */
struct tm Ntp::GetTime()
{
    if (!isSynchronized)
    {
        LOGW(TAG, "Time is Not SET, Please call SetAndSyncTime before trying to get local time");
        return timeinfo;
    }
    time(&now);
    localtime_r(&now, &timeinfo);
    char buf[64];
    strftime(buf, sizeof(buf), "%c", &timeinfo);
    strftime_buf = buf;
    LOG_NTP_V(TAG, "The current date/time is: %s", strftime_buf.c_str());
    return timeinfo;
}
/**
 * @brief return ntp status 
 * 
 * @return true  time synced with ntp server
 * @return false 
 */
bool Ntp::IsSyncronised() const
{
    return isSynchronized;
}

/**
 * @brief print the time 
 * 
 * @param time_struct 
 */
void Ntp::PrintTime()
{
    GetTime();
    ESP_LOGI(TAG, "The current synced date/time in UTC is: %s", strftime_buf.c_str());
}
/**
 * @brief get time stamp 
 * 
 * @param time_struct 
 */
unsigned long Ntp::GetTimeStamp()
{
    if (!isSynchronized)
    {
        LOGE(TAG, "Time is Not SET yet ..");
        return 0;
    }
    GetTime();
    LOG_NTP_V(TAG, "timestamp :%ld", (long)now);
    return (unsigned long)now;
}

/**
 * @brief get time in form of string ( time and date )
 * 
 * @return std::string 
 */
std::string Ntp::GetTimeString() const
{
    return Ntp::strftime_buf;
}

/**
 * @brief 
 * 
 * @return unsigned long 
 */
unsigned long Ntp::GetStartupTimeStamb() const
{
    return SyncTimeStamb;
}

/**
 * @brief 
 * 
 * @return esp_err_t 
 */
esp_err_t Ntp::Diagnose()
{
    if (!busy && isSynchronized)
    {
        if (sntp_get_sync_status() == SNTP_SYNC_STATUS_COMPLETED)
        {
            return ESP_OK;
        }
    }
    return ESP_FAIL;
}

esp_err_t Ntp::SetConfigurationParameters(const json &config_in)
{
    esp_err_t ret = ESP_FAIL;
    if (config_in.contains(TAG) != 0)
    {
        if (config_in[TAG].contains("params") != 0)
        {
            auto ntpConfig = config_in[TAG]["params"];
            AssertjsonStr(ntpConfig, "ntpServer", ntpServer, 5);
            if (isConfigured)
            {
                return SaveToNVS();
            }
        }
    }
    return ret;
}

esp_err_t Ntp::GetConfiguration(json &config_out) const
{
    try
    {
        config_out[TAG]["params"] =
            {
                {"ntpServer", ntpServer.c_str()},
            };
        return ESP_OK;
    }
    catch (const std::exception &e)
    {
        ESP_LOGE(TAG, "%s", e.what());
        return ESP_FAIL;
    }
}

esp_err_t Ntp::GetConfigurationStatus(json &config_out) const
{
    try
    {
        config_out[TAG]["status"] =
            {
                {"isConfigured", isConfigured},
                {"isSynced", isSynchronized},
            };
        return ESP_OK;
    }
    catch (const std::exception &e)
    {
        ESP_LOGE(TAG, "%s", e.what());
        return ESP_FAIL;
    }
}

esp_err_t Ntp::RestoreDefault()
{

    ntpServer = CONFIG_NTP_DEFAULT_SERVER; // (1000 * 60 * 60 * OTA_DELAY_HOUR);
    isConfigured = false;
    SaveToNVS();
    return ESP_OK;
}

esp_err_t Ntp::LoadFromNVS()
{
    auto nvs = std::make_unique<NVS>(this->TAG);
    esp_err_t ret = ESP_FAIL;
    if (nvs->isOpen())
    {
        ret = nvs->getS("ntpServer", ntpServer);
        ret |= nvs->get("isConfigured", isConfigured);
    }
    return ret;
}

esp_err_t Ntp::SaveToNVS()
{
    auto nvs = std::make_unique<NVS>(this->TAG, NVS_READWRITE);
    esp_err_t ret = ESP_FAIL;
    if (nvs->isOpen())
    {
        ret = nvs->setS("ntpServer", ntpServer.c_str());
        ret |= nvs->set("isConfigured", isConfigured);
    }
    return ret;
}

std::shared_ptr<Ntp> ntp;