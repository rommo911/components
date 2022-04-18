#ifndef WIFI_DIRECTORY__H
#define WIFI_DIRECTORY__H
#pragma once

#include "esp_err.h"
#include <list>
#include <string.h>
#include <string>
#include <memory>
using namespace std;
class WifiDirectory
{
    friend class Wifi;

public:
    static constexpr char TAG[] = "WIFI Directory";
    struct HotspotEntry
    {
        std::string ssid;
        std::string password;
        std::string bssid;
    };
    WifiDirectory();
    ~WifiDirectory();
    esp_err_t Init();
    esp_err_t CreateEntry(const HotspotEntry &tempHotspot);
    esp_err_t DeleteEntry(const char *ssid);
    uint8_t GetListNumber();
    void PrintSavedSSID();
    void PrintEntry(const HotspotEntry &entry);
    esp_err_t Save();
    //////////////webserver wrapping function //////
    char *StructToJson(const HotspotEntry &entry);
    HotspotEntry JsonToStruc(const char *entryJson);
    const auto& GetList()
    {
        return wifiList;
    }

protected:
    esp_err_t ResetDirectory();

private:
    bool isInitialized = false;
    static const uint16_t BUFFER_SIZE = CONFIG_WIFI_DIRECTORY_READ_BUFFER_SIZE;
    bool isModified = false;
    static constexpr char check[] = "ABCDEFGHIGKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz+0123465798'()@[]=<>!:+-/* $~&_";
    static const int LIST_SIZE = CONFIG_WIFI_DIRECTORY_LIST_SIZE;
    list<WifiDirectory::HotspotEntry> wifiList;
};
extern std::unique_ptr<WifiDirectory> wifiDirectory;
#endif