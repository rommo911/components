/**
 * @file Ota.h
 * @author your name (you@domain.com)
 * @brief 
 * @version 0.1
 * @date 2020-10-30
 * 
 * @copyright Copyright (c) 2020
 * 
 */

#ifndef __OTA_H
#define __OTA_H
#pragma once

#include "system.hpp"
#include "task_tools.h"
#ifdef ESP_PLATFORM
#include "esp_ota_ops.h"
#endif // DEBUG
#include <freertos/event_groups.h>
#include <string>

//#define ALLOW_DOWNGRADE
#define CONFIG_DEFAULT_URL "ota.domalys.com"
/*
extern const uint8_t server_cert_pem_start[] asm("_binary_ca_cert_pem_start");
extern const uint8_t server_cert_pem_end[] asm("_binary_ca_cert_pem_end");
*/
class Ota;

typedef esp_err_t (*ota_cb_t)(void *arg);

class Ota : public Task
{
private:
    static constexpr char TAG[] = "ota";
    EventLoop_p_t Loop;
    static const unsigned long OTA_FETCH_DELAY = 3000;
    std::string URL = ("");
    std::string host = ("");
    unsigned int port = 0;
    unsigned int rebootDelay = 0;
    unsigned int bufferSize = 0;
    unsigned int receiveTimeout = 0;
    unsigned long otaDelay = 0; // (1000 * 60 * 60 * OTA_DELAY_HOUR);
    bool isBusy = false;
    bool isRunning = false;
    bool checkVersion =false;

public:
    const Event_t EVENT_OTA_START_CHEKING_URL = {TAG, EventID_t(6)};
    const Event_t EVENT_OTA_FETCHING = {TAG, EventID_t(0)};
    const Event_t EVENT_FORCE_OTA = {TAG, EventID_t(1)};
    const Event_t EVENT_FAILED = {TAG, EventID_t(2)};
    const Event_t EVENT_NEW_VERSION_FOUND = {TAG, EventID_t(3)};
    const Event_t EVENT_NEW_VERSION_FLASHED = {TAG, EventID_t(4)};
    
    Ota(EventLoop_p_t &);
    ~Ota()
    {
        StopTask();
    };
    //set active url
    esp_err_t SetUrl(const std::string &url);
    ota_cb_t OtaPreFlashCallback{nullptr};
    ota_cb_t OtaPostFlashCallback{nullptr};
    //set callback to be called before flashing
    esp_err_t SetPreOtaCb(ota_cb_t cb);
    //set callback to be called before flashing
    esp_err_t SetPostOtaCb(ota_cb_t cb);
    //set time interval to check for ota (usually 24H)
    esp_err_t SetTimer(unsigned long delay);
    //start ota listner (task that checks for ota and flash/reboot each interval)
    esp_err_t StartOtaService();
    //
    esp_err_t StopOtaService();
    //
    esp_app_desc_t GetCurrentVersion();
    //CONFIG OVERRIDE
private:
    // this will check MD5 and compatibility
    esp_err_t validate_image_header(esp_app_desc_t *new_app_info, bool checkVer = true);
    //timeout for http req
    esp_err_t DoOTA(bool checkVer = true, char *cert = NULL);
    //ota deamon
    void run(void *arg);
};
typedef std::unique_ptr<Ota> OTA_t;

extern OTA_t ota;

#endif
