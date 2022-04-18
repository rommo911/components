/**
 * @file web_server.cpp
 * @author RAMI ZAYAT
 * @brief WebClient fonctionality packaged in the class
 * @version 0.1
 * @date 2020-08-18
 * 
 * @copyright Copyright (c) 2020
 * 
 */

#include "debug_tools.h"

#if (DEBUG_WEBCLIENT == VERBOS)
#define LOG_WEBCLIENT LOGD
#define LOG_WEBCLIENT_V LOGV
#elif (DEBUG_WEBCLIENT == INFO)
#define LOG_WEBCLIENT LOGI
#define LOG_WEBCLIENT_V LOGD
#else
#define LOG_WEBCLIENT LOGD
#define LOG_WEBCLIENT_V LOGV
#endif

#include "webclient.hpp"
#include "esp_log.h"
#include "esp_tls.h"
#include <ctype.h>
#include <string.h>

////////////////////////////////////////////////////////////////
///
/// static variables, constants and configuration assignments
///
/// ////////////////////////////////////////////////////////////

/**
 * @brief initialize the WebClient and hook it with an event group
 * 
 * 
 * @return esp_err_t 
 */
esp_err_t WebClient::Init()
{
    LOG_WEBCLIENT(TAG, "Init Started");
    if (isStarted)
    {
        LOGE(TAG, "already initialized and running !");
        return ESP_OK;
    }
    return ESP_OK;
}

/**
 * @brief check WebClient status
 * 
 * @return true WebClient is running
 * @return false WebClient is not running
 */
bool WebClient::IsStarted()
{
    return isStarted;
}

/**
 * @brief start a WebClient http request with some assertion
 * 
 * @return esp_err_t 
 */
esp_err_t WebClient::StartGET(const esp_http_client_config_t &config, bool thread)
{
    clientConfig = config;
    if (thread)
    {
        return StartGetThread();
    }
    client = esp_http_client_init(&clientConfig);
    //check if request was made to return data or not (allocated external buffer)
    isStarted = true;
    esp_err_t err = esp_http_client_perform(client);
    if (err == ESP_OK)
    {
        LOG_WEBCLIENT(TAG, "HTTP GET Status = %d, content_length = %d",
                      esp_http_client_get_status_code(client),
                      esp_http_client_get_content_length(client));
        esp_http_client_cleanup(client);
        return ESP_OK;
    }
    else
    {
        LOGE(TAG, "HTTP GET request failed: %s", esp_err_to_name(err));
        esp_http_client_cleanup(client);
        return err;
    }
}

esp_err_t WebClient::StartPOST(const esp_http_client_config_t &config, const char *postdata, const char *contentType, bool thread)
{
    if (postdata == NULL || !strlen(postdata))
    {
        LOGE(TAG, "post data assert failed");
        return ESP_FAIL;
    }
    clientConfig = config;
    if (thread)
    {
        PostHelper1 = postdata;
        PostHelper2 = contentType;
        return StartPostThread();
    }
    client = esp_http_client_init(&clientConfig);
    //    esp_http_client_set_url(client, config.url);
    esp_http_client_set_method(client, HTTP_METHOD_POST);
    esp_http_client_set_header(client, "Content-Type", contentType); //"application/json"
    esp_http_client_set_post_field(client, postdata, strlen(postdata));
    isStarted = true;
    esp_err_t err = esp_http_client_perform(client);
    if (err == ESP_OK)
    {
        LOG_WEBCLIENT(TAG, "HTTP POST Status = %d, content_length = %d",
                      esp_http_client_get_status_code(client),
                      esp_http_client_get_content_length(client));
        esp_http_client_cleanup(client);
        return ESP_OK;
    }
    else
    {
        LOGE(TAG, "HTTP POST request failed: %s", esp_err_to_name(err));
        esp_http_client_cleanup(client);
        return err;
    }
}

/**
 * @brief stop the WebClient 
 * 
 * @return esp_err_t 
 */
esp_err_t WebClient::Stop()
{
    if (isStarted == false)
    {
        LOGE(TAG, "Error client not running");
        return ESP_FAIL;
    }
    isStarted = false;
    return ESP_OK;
}

/**
 * @brief decode the url string (replace bad stuff)
 * 
 * @param str 
 * @return char* 
 */
std::string WebClient::urlDecode(const char *str)
{
    char *dStr = (char *)malloc(strlen(str) + 1);
    char eStr[] = "00"; /* for a hex code */

    strcpy(dStr, str);
    int i; /* the counter for the string */

    for (i = 0; i < strlen(dStr); ++i)
    {

        if (dStr[i] == '%')
        {
            if (dStr[i + 1] == 0)
                return dStr;

            if (isxdigit(dStr[i + 1]) && isxdigit(dStr[i + 2]))
            {
                /* combine the next to numbers into one */
                eStr[0] = dStr[i + 1];
                eStr[1] = dStr[i + 2];
                /* convert it to decimal */
                long int x = strtol(eStr, NULL, 16);
                /* remove the hex */
                memmove(&dStr[i + 1], &dStr[i + 3], strlen(&dStr[i + 3]) + 1);
                dStr[i] = x;
            }
        }
        else if (dStr[i] == '+')
        {
            dStr[i] = ' ';
        }
    }
    std::string ret = dStr;
    free(dStr);
    return ret;
}

esp_err_t WebClient::generic_http_event_handle(esp_http_client_event_t *evt)
{
    EventGroupHandle_t HttpClientEvent = NULL;
    DynamicBuffer_t *buffer = NULL;
    if (evt->user_data != NULL)
    {
        buffer = (DynamicBuffer_t *)evt->user_data;
        HttpClientEvent = buffer->HTTPEventGroup;
    }
    static char *output_buffer; // Buffer to store response of http request from event handler
    static int output_len;      // Stores number of bytes read
    switch (evt->event_id)
    {
    case HTTP_EVENT_ERROR:
        LOGE(TAG, "HTTP_EVENT_ERROR");
        break;
    case HTTP_EVENT_ON_CONNECTED:
        LOG_WEBCLIENT(TAG, "HTTP_EVENT_ON_CONNECTED");
        break;
    case HTTP_EVENT_HEADER_SENT:
        LOG_WEBCLIENT(TAG, "HTTP_EVENT_HEADER_SENT");
        break;
    case HTTP_EVENT_ON_HEADER:
        ESP_LOGV(TAG, "HTTP_EVENT_ON_HEADER");
        printf("%.*s", evt->data_len, (char *)evt->data);
        break;
    case HTTP_EVENT_ON_DATA:
        if (HttpClientEvent != NULL)
            xEventGroupSetBits(HttpClientEvent, WEB_CLIENT_DATA_BIT);
        ESP_LOGV(TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
        /*
             *  Check for chunked encoding is added as the URL for chunked encoding used in this example returns binary data.
             *  However, event handler can also be used in case chunked encoding is used.
             */
        if (!esp_http_client_is_chunked_response(evt->client))
        {
            // If user_data buffer is configured, copy the response into the buffer

            if (buffer != NULL && buffer->pointer != NULL && output_len < buffer->size)
            {
                memcpy(buffer->pointer + output_len, evt->data, evt->data_len);
            }
            else
            {
                if (output_buffer == NULL)
                {
                    output_buffer = (char *)malloc(esp_http_client_get_content_length(evt->client));
                    output_len = 0;
                    if (output_buffer == NULL)
                    {
                        LOGE(TAG, "Failed to allocate memory for output buffer");
                        return ESP_FAIL;
                    }
                }
                memcpy(output_buffer + output_len, evt->data, evt->data_len);
            }
            output_len += evt->data_len;
        }
        break;
    case HTTP_EVENT_ON_FINISH:
        LOG_WEBCLIENT(TAG, "HTTP_EVENT_ON_FINISH");
        if (output_buffer != NULL)
        {
            // Response is accumulated in output_buffer. Uncomment the below line to print the accumulated response
            LOG_WEBCLIENT(TAG, "%.*s", output_len, output_buffer);
            free(output_buffer);
            output_buffer = NULL;
        }
        output_len = 0;
        if (HttpClientEvent != NULL)
            xEventGroupSetBits(HttpClientEvent, WEB_CLIENT_STOPPED_BIT);
        break;
    case HTTP_EVENT_DISCONNECTED:
        LOG_WEBCLIENT(TAG, "HTTP_EVENT_DISCONNECTED");
        int mbedtls_err = 0;
        esp_err_t err = esp_tls_get_and_clear_last_error((esp_tls_error_handle_t)evt->data, &mbedtls_err, NULL);
        if (err != 0)
        {
            if (output_buffer != NULL)
            {
                free(output_buffer);
                output_buffer = NULL;
            }
            output_len = 0;
            LOG_WEBCLIENT(TAG, "Last esp error code: 0x%x", err);
            LOG_WEBCLIENT(TAG, "Last mbedtls failure: 0x%x", mbedtls_err);
        }
        if (HttpClientEvent != NULL)
            xEventGroupSetBits(HttpClientEvent, WEB_CLIENT_DISCONNECTED_BIT);
        break;
    }
    return ESP_OK;
}

void WebClient::HttpGetThreadTask(void *arg)
{
    if (arg != NULL)
    {
        WebClient *client = (WebClient *)arg;
        client->StartGET(client->clientConfig);
        client->getTaskHandle = nullptr;
    }
    vTaskDelete(NULL);
}

void WebClient::HttpPostThreadTask(void *arg)
{
    if (arg != NULL)
    {
        WebClient *client = (WebClient *)arg;
        client->StartPOST(client->clientConfig, client->PostHelper1.c_str(), client->PostHelper2.c_str(), false);
        client->postTaskHandle = nullptr;
    }
    vTaskDelete(NULL);
}

esp_err_t WebClient::StartGetThread()
{
    if (getTaskHandle == NULL)
    {
        xTaskCreate(HttpGetThreadTask, "HTTP PSOT", TASK_STACK, this, TASK_PRIORITY, &getTaskHandle);
        return ESP_OK;
    }
    return ESP_FAIL;
}

esp_err_t WebClient::StartPostThread()
{
    if (postTaskHandle == NULL)
    {
        xTaskCreate(HttpPostThreadTask, "HTTP PSOT", TASK_STACK, this, TASK_PRIORITY, &postTaskHandle);
        return ESP_OK;
    }
    return ESP_FAIL;
}