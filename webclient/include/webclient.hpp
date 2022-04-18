/**
 * @file aladin_webserver.h
 * @author rami
 * @brief
 * @version 0.1
 * @date 2020-09-23
 *
 * @copyright Copyright (c) 2021
 *
 */

#ifndef WEB_CLIENT_H
#define WEB_CLIENT_H
#pragma once
#ifdef ESP_PLATFORM
#include <esp_http_client.h>
#endif // ESP_PLATFORM#
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"
#include <string>
class WebClient
{
    protected:
    static constexpr char TAG[] = "WebClient";

    public:
    struct DynamicBuffer_t
    {
        char* pointer;
        size_t size;
        EventGroupHandle_t HTTPEventGroup;
    };
    WebClient();
    ~WebClient();
    //check if an http request is running at this moment
    bool IsStarted();
    //initialize and config the client parameters
    esp_err_t Init();
    //start an http GET request
    esp_err_t StartGET(const esp_http_client_config_t& config, bool thread = false);
    //start an http POST
    esp_err_t StartPOST(const esp_http_client_config_t& config, const char* postdata, const char* contentType, bool thread = false);
    //
    esp_err_t Stop();
    //
    static esp_err_t generic_http_event_handle(esp_http_client_event_t* evt);

    static const uint8_t WEB_CLIENT_STARTED_BIT = BIT(0);
    static const uint8_t WEB_CLIENT_STOPPED_BIT = BIT(1);
    static const uint8_t WEB_CLIENT_ERROR_BIT = BIT(2);
    static const uint8_t WEB_CLIENT_DISCONNECTED_BIT = BIT(3);
    static const uint8_t WEB_CLIENT_DATA_BIT = BIT(3);

    private:
    static const uint16_t TASK_STACK = 4096;
    static const uint8_t TASK_PRIORITY = 4;
    esp_http_client_handle_t client;
    esp_http_client_config_t clientConfig;
    static void HttpGetThreadTask(void* arg);
    static void HttpPostThreadTask(void* arg);
    esp_err_t StartGetThread();
    esp_err_t StartPostThread();
    TaskHandle_t postTaskHandle;
    TaskHandle_t getTaskHandle;
    std::string PostHelper1, PostHelper2;
    bool isStarted;
    //utility :  decode url string ( replace the %xx stuff with correct character)
    static std::string urlDecode(const char* str);
};

#endif