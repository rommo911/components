#ifndef NTP_H
#define NTP_H
#include "config.hpp"
#include "esp_err.h"
#include "sdkconfig.h"
#include "sntp.h"
#include <string>
#include <time.h>

class Ntp : public Config
{

private:
    static constexpr char TAG[] = "ntp";
    static bool isSynchronized;
    static bool busy;
    static void TimeSyncNotificationCallback(struct timeval *tv);
    std::string strftime_buf;
    time_t now = 0;
    std::string ntpServer;
    static unsigned long SyncTimeStamb;

public:
    Ntp();
    struct tm timeinfo;

    bool IsSyncronised() const;
    //get tm struct current time
    struct tm GetTime();
    //set server and sync time
    esp_err_t SetAndSyncTime(const char *server = CONFIG_NTP_DEFAULT_SERVER);
    //dump to serial the time & date
    void PrintTime();
    //get timestamb
    unsigned long GetTimeStamp();
    //date & time string , updated each time get is called
    std::string GetTimeString() const;
    //get the first epoch timestamb that gives rami boot time;
    unsigned long GetStartupTimeStamb() const;

    //CONFIG OVERRIDE
    esp_err_t Diagnose();
    esp_err_t SetConfigurationParameters(const json &config_in) override;
    esp_err_t GetConfiguration(json &config_out) const override;
    esp_err_t GetConfigurationStatus(json &config_out) const override;

    esp_err_t RestoreDefault() override;
    esp_err_t LoadFromNVS() override;
    esp_err_t SaveToNVS() override;
};

extern std::shared_ptr<Ntp> ntp;
#endif