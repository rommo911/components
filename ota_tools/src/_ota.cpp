#include "_ota.h"
#include "debug_tools.h"
#include "nvs_tools.h"
#include "utilities.hpp"
#include "esp_http_client.h"
#include "esp_https_ota.h"
#include "esp_log.h"
#include "esp_partition.h"
#include "esp_system.h"
#include "string.h"


Ota::Ota(EventLoop_p_t& _loop) : Task(TAG, CONFIG_OTA_TASK_STACK_SIZE, CONFIG_OTA_TASK_PRIORITY, CONFIG_OTA_TASK_CORE),
Loop(_loop),
host("esp")
{
    URL = CONFIG_DEFAULT_URL;
    host = "esp";
    port = CONFIG_OTA_HTTPS_DEFAULT_PORT;
    rebootDelay = CONFIG_OTA_DELAY_BEFORE_REBOOT_MS;
    bufferSize = CONFIG_OTA_BUFFER_SIZE;
    receiveTimeout = CONFIG_OTA_RECV_TIMEOUT_MS;
    otaDelay = CONFIG_OTA_DEFAULT_CHECK_DELAY_MINUTES * 1000 * 60; // (1000 * 60 * 60 * OTA_DELAY_HOUR);
}

esp_app_desc_t Ota::GetCurrentVersion()
{
    const esp_partition_t* running = esp_ota_get_running_partition();
    esp_app_desc_t running_app_info = {};
    esp_ota_get_partition_description(running, &running_app_info);
    return running_app_info;
}

/**
 * @brief
 *
 * @param new_app_info
 * @param checkVer
 * @return esp_err_t
 */
esp_err_t Ota::validate_image_header(esp_app_desc_t* new_app_info, bool checkVer)
{
    if (new_app_info == NULL)
    {
        return ESP_FAIL;
    }
    const esp_partition_t* running = esp_ota_get_running_partition();
    esp_app_desc_t running_app_info;
    if (esp_ota_get_partition_description(running, &running_app_info) == ESP_OK)
    {
        ESP_LOGI(TAG, "Running firmware version: %s", running_app_info.version);

        std::string currentVersion = running_app_info.version;
        std::string newVersion = new_app_info->version;
        if (checkVer)
        {
            float currentV = 0;
            float NewV = 0;

            try
            {
                currentV = std::stof(currentVersion);
                NewV = std::stof(newVersion);
            }
            catch (const std::exception& e)
            {
            }
            if (currentV == NewV)
            {
                //throw(ESPException("same version ", TAG));
                ESP_LOGE(TAG, " Version is same as existing ! ");
                return ESP_ERR_INVALID_VERSION;
            }
#ifndef ALLOW_DOWNGRADE
            if (currentV < NewV)
            {
                // throw(ESPException("version is older than flashed , prohibited ", TAG));
                ESP_LOGE(TAG, " Version is odler than existing ! ");
                return ESP_ERR_INVALID_VERSION;
            }
#endif
            Loop->post_event(EVENT_NEW_VERSION_FOUND);
        }
    }
    return ESP_OK;
}

/**
 * @brief start ota update, internal use, dont call in runtime.
 *  this will start the ota, check header and version and validate it
 * flash its partition and return ESP_OK to its handler (task) which will restart
 *
 * @param url
 * @param checkVer
 * @param cert
 * @return esp_err_t
 */
esp_err_t Ota::DoOTA(bool checkVer, char* cert)
{
    if (URL.length() < 10 || isBusy == true)
    {
        ESP_LOGI(TAG, " url not set ");
        return ESP_FAIL;
    }
    std::string errorStr = "";
    isBusy = true;
    ESP_LOGI(TAG, "Starting do OTA check");
    esp_err_t ota_finish_err = ESP_OK;
    esp_http_client_config_t http_ota;
    memset(&http_ota, 0, sizeof(http_ota));
    http_ota.url = URL.c_str();
    http_ota.host = host.c_str();
    http_ota.cert_pem = cert;
    http_ota.buffer_size = bufferSize;
    http_ota.timeout_ms = receiveTimeout;
    http_ota.transport_type = HTTP_TRANSPORT_OVER_TCP;
    http_ota.port = port;
    http_ota.skip_cert_common_name_check = true;
    esp_https_ota_config_t ota_config;
    memset(&ota_config, 0, sizeof(ota_config));
    ota_config.http_config = &http_ota;
    esp_https_ota_handle_t https_ota_handle{};
    //esp_http_client_set_header(https_ota_handle.)
    ESP_LOGI(TAG, "OTA url : %s  : %d , %d", ota_config.http_config->url, http_ota.port, http_ota.timeout_ms);
    esp_err_t err = esp_https_ota_begin(&ota_config, &https_ota_handle);
    Loop->post_event(EVENT_OTA_START_CHEKING_URL);
    if (err != ESP_OK)
    {
        errorStr = "ESP HTTPS OTA failed";
        goto ota_end;
    }
    ESP_LOGI(TAG, "found OTA link ok .. cheking binary version");
    esp_app_desc_t app_desc;
    err = esp_https_ota_get_img_desc(https_ota_handle, &app_desc);
    if (err != ESP_OK)
    {
        errorStr = "image header validate failed";
        goto ota_end;
    }
    err = validate_image_header(&app_desc, checkVer);
    if (err != ESP_OK)
    {
        errorStr = "image header verification failed , or version is not new ";
        goto ota_end;
    }
    else
    {
        Loop->post_event(EVENT_NEW_VERSION_FOUND);
        if (OtaPreFlashCallback != NULL)
        {
            if (OtaPreFlashCallback(this) != ESP_OK)
            {
                errorStr = "couldn'not execute pre ota callback";
                goto ota_end;
            }
        }
    }
    while (1) // adde some optimisation
    {
        size_t fetched = esp_https_ota_get_image_len_read(https_ota_handle);
        ESP_LOGD(TAG, "Image bytes read: %d", fetched);
        Loop->post_event_data(EVENT_OTA_FETCHING, fetched);
        err = esp_https_ota_perform(https_ota_handle);
        if (err != ESP_ERR_HTTPS_OTA_IN_PROGRESS)
        {
            break;
        }
    }
    if (esp_https_ota_is_complete_data_received(https_ota_handle) != true)
    {
        errorStr = "Complete data was not received.";
    }
ota_end:
    ota_finish_err = esp_https_ota_finish(https_ota_handle);
    if ((err == ESP_OK) && (ota_finish_err == ESP_OK))
    {
        ESP_LOGI(TAG, "ESP_HTTPS_OTA upgrade successful");
        isBusy = false;
        return ESP_OK;
    }
    else
    {
        if (ota_finish_err == ESP_ERR_OTA_VALIDATE_FAILED)
        {
            LOGI(TAG, "No new version found");
            return ESP_OK;
        }
        LOGW(TAG, "ESP_HTTPS_OTA upgrade failed %d,   %s ", ota_finish_err, errorStr.c_str());
        Loop->post_event_data(EVENT_FAILED, ota_finish_err);
        isBusy = false;
        return ESP_FAIL;
    }
}

/**
 * @brief set a callback to be executed before flashing, like suspend all current operations
 *
 * @param cb
 * @return esp_err_t
 */
esp_err_t Ota::SetPreOtaCb(ota_cb_t cb)
{
    if (cb != NULL)
    {
        OtaPreFlashCallback = cb;
        return ESP_OK;
    }
    return ESP_FAIL;
}

/**
 * @brief
 *
 * @param cb
 * @return esp_err_t
 */
esp_err_t Ota::SetPostOtaCb(ota_cb_t cb)
{
    if (cb != NULL)
    {
        OtaPostFlashCallback = cb;
        return ESP_OK;
    }
    return ESP_FAIL;
}

/**
 * @brief create the ota task and its events group
 *
 * @return esp_err_t
 */
esp_err_t Ota::StartOtaService()
{
    if (StartTask(this) != ESP_OK)
    {
        LOGE(TAG, " Something went wrong ..");
        return ESP_FAIL;
    }
    return ESP_OK;
}

/**
 * @brief stop the task and free
 *
 * @return esp_err_t
 */
esp_err_t Ota::StopOtaService()
{
    if (isBusy)
    {
        LOGE(TAG, "Cannot stop Listner , ota in progress");
    }
    else if (TaskIsRunning())
    {
        StopTask();
        isBusy = false;
        return ESP_OK;
    }
    return ESP_FAIL;
}

/**
 * @brief running task will check for ota each intervale and reboot when needed
 *
 * @param arg
 */
void Ota::run(void* arg)
{
    esp_err_t ret;
    while (1)
    {
        EventHandlerSync_t otaScheduler(Loop);
        otaScheduler.listen_to(EVENT_FORCE_OTA);
        otaScheduler.wait_event_for(std::chrono::seconds(otaDelay));
        ctlLock->lock("run");
        try
        {
            ret = DoOTA(false);
            if (ret == ESP_OK)
            {
                if (OtaPostFlashCallback != NULL)
                {
                    OtaPostFlashCallback(this);
                }
                LOGI(TAG, "OTA success , will reboot in 15s ");
                Loop->post_event(EVENT_NEW_VERSION_FLASHED);
                std::this_thread::sleep_for(std::chrono::seconds(15));
                esp_restart();
            }
        }
        catch (const std::exception& e)
        {
            LOGE(TAG, " - %s", e.what());
        }
        ctlLock->unlock();
    }
}

/**
 * @brief set the ota check interval, will not ignore current delay.
 *
 * @param delay
 * @return esp_err_t
 */
esp_err_t Ota::SetTimer(unsigned long delay)
{
    if (delay > 10000 && delay < (24 * 60 * 60 * 1000) /** one day max*/)
    {
        otaDelay = delay;
        Loop->post_event(EVENT_FORCE_OTA);
        return ESP_OK;
    }
    else
    {
        return ESP_ERR_INVALID_ARG;
    }
}

OTA_t ota = nullptr;