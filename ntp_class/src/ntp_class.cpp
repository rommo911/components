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


#include "ntp_class.h"
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

Ntp::Ntp()
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
        busy = true;
        if (server == NULL)
            return ESP_ERR_INVALID_ARG; // assert
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
        ESP_LOGW(TAG, "Time is Not SET, Please call SetAndSyncTime before trying to get local time");
        return timeinfo;
    }
    time(&now);
    localtime_r(&now, &timeinfo);
    char buf[64];
    strftime(buf, sizeof(buf), "%c", &timeinfo);
    strftime_buf = buf;
    ESP_LOGI(TAG, "The current date/time is: %s", strftime_buf.c_str());
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
        ESP_LOGE(TAG, "Time is Not SET yet ..");
        return 0;
    }
    GetTime();
    ESP_LOGI(TAG, "timestamp :%ld", (long)now);
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

std::shared_ptr<Ntp> ntp;