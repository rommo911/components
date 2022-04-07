#include "boot_cfg.h"
#include "debug_tools.h"
#include "nvs_tools.h"
#include "esp32-hal-psram.h"
#include "esp_log.h"
#include "esp_partition.h"
#include "esp_system.h"

#include <list>
#include <memory>
AladinStatus_t BootConfig_t::lastStatus;

#define MAX_RESET_FACTORY "MaxRestFac"
#define IS_NEED_VALIDATION "IsNeedValidation"
#define PS_RAM_OK "psRamOK"
#define IS_ROLLED_BACK "IsRolledBack"
#define LAST_STATUS "lastStatus"
#define OTA_STATUS "otaState"
#define REBOOT_COUNTER "rebootCounter"
#define BOOT_REASON "bootReason"

/**
 * @brief reset to factory APP partition and reboot ( force)
 *
 */
esp_err_t BootConfig_t::RestoreFactory()
{
    const esp_partition_t *factoryApp = esp_partition_find_first(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_FACTORY, NULL);
    if (factoryApp != NULL)
    {
        esp_ota_set_boot_partition(factoryApp); //! FACTORY RESET
        rebootCounter = 0;
        NvsPointer storage = OPEN_NVS_W(TAG);
        storage->set("rebootCounter", rebootCounter);
        otaState = ESP_OTA_IMG_UNDEFINED;
        esp_restart();
    }
    return ESP_OK;
}
/**
 * @brief update the otaState variable to the current partition
 *
 */
esp_err_t BootConfig_t::CheckCurrentOTAPartitionStatus() const
{
    const esp_partition_t *_RunningPartition = esp_ota_get_running_partition();
    runningPartition = (esp_partition_t *)_RunningPartition;
    return esp_ota_get_state_partition(runningPartition, &otaState);
}

/**
 * @brief start boot sequence check and diagnose
 *
 * @return esp_err_t
 */
esp_err_t BootConfig_t::Boot()
{
    InitThisConfig();
    if (IsFactoryRolledBack())
    {
        ESP_LOGW(TAG, "Rollback took place ! booted FACTORY partition");
    }
    if (IsOTARolledBack())
    {
        ESP_LOGW(TAG, "Rollback took place ! booted last good partition");
    }
    bootReason = rtc_get_reset_reason(0);
    std::string reasonStr = GetResetReasonString(bootReason);
    // check if a reason is a power
    if (bootReason == POWERON_RESET)
    {

        AddWarningCode(GetResetReasonString(bootReason) + "reboot counter = " + std::to_string(rebootCounter));
        rebootCounter = 0;
    }
    else if (rebootCounter > MaxRebootCountTriggerFactory && bootReason == SW_RESET) // check if too much reboot occured
    {
        // RestoreFactory(); //TODO
    }
    // check if a reason is a positive crash to increment counter (positive= its very sure)
    for (const auto &reason : ErrorReason) // ErrorReason is list of reason that can be for a crash
    {
        if (bootReason == reason)
        {
            rebootCounter++;
            AddErrorCode("Reboot Reason :" + reasonStr);
            break;
        }
    }
    if (otaState == ESP_OTA_IMG_PENDING_VERIFY)
    {
        ESP_LOGW(TAG, "new OTA image Pending validation");
        AddWarningCode("new OTA image Pending verify :" + reasonStr);
    }
    else
    {
        rebootCounter++;
        AddErrorCode("Reboot Reason:" + reasonStr);
    }
    SaveToNVS(); // save the counter and current status
    ESP_LOGW("Reboot Reason:", "%s , count=%d", reasonStr.c_str(), rebootCounter);
    return ESP_OK;
}

/**
 * @brief check active ota app status if need validation or not
 *
 * @return true need validation ( will rollback if not validate)
 * @return false
 */
bool BootConfig_t::IsNeedValidation() const
{
    return (otaState == ESP_OTA_IMG_PENDING_VERIFY);
}

bool BootConfig_t::CheckInvalidOtaState() const
{
    const esp_partition_t *_invalidPartition = esp_ota_get_last_invalid_partition();
    auto _runningPartition = (esp_partition_t *)_invalidPartition;
    auto invalid_otaState = ESP_OTA_IMG_UNDEFINED;
    esp_err_t ret = esp_ota_get_state_partition(_runningPartition, &invalid_otaState);
    return ((invalid_otaState == ESP_OTA_IMG_ABORTED) && ret == ESP_OK);
}
/**
 * @brief check if this image is the rolled back app image
 *
 * @return true
 * @return false
 */
bool BootConfig_t::IsOTARolledBack() const
{
    if (CheckCurrentOTAPartitionStatus() != ESP_ERR_NOT_SUPPORTED)
    {
        return CheckInvalidOtaState();
    }
    return false;
}
bool BootConfig_t::IsFactoryRolledBack() const
{
    const auto running_app = esp_ota_get_running_partition();
    if (running_app != NULL && running_app->type == ESP_PARTITION_TYPE_APP && running_app->subtype == ESP_PARTITION_SUBTYPE_APP_FACTORY)
    {

        return CheckInvalidOtaState();
    }
    else
    {
        return false;
    }
}

/**
 * @brief confirm the running ota app is ok and cancel rollback
 *
 */
esp_err_t BootConfig_t::SetValidOtaImage()
{
    esp_err_t ret = ESP_ERR_INVALID_STATE;
    if (IsNeedValidation())
    {
        otaState = ESP_OTA_IMG_VALID;
        ret = esp_ota_mark_app_valid_cancel_rollback();

        if (ret == ESP_OK)
        {
            CheckCurrentOTAPartitionStatus();
            ESP_LOGW(TAG, "new OTA image validated !! ");
            NvsPointer storage = OPEN_NVS_W(TAG);
            storage->set("otaState", (int)otaState);
        }
    }
    return ret;
}

esp_err_t BootConfig_t::EraseInvalidApp()
{
    const esp_partition_t *_invalidPartition = esp_ota_get_last_invalid_partition();
    if (_invalidPartition != NULL)
    {
        esp_ota_erase_last_boot_app_partition();
    }
    else
    {
        ESP_LOGE(TAG, "no last invalid app ");
    }
    return ESP_ERR_INVALID_STATE;
}

/**
 * @brief set this image is NOT ok , reboot and rollback to last ota app
 *
 */
void BootConfig_t::SetInvalidOtaImageAndReboot()
{
    otaState = ESP_OTA_IMG_INVALID;
    NvsPointer storage = OPEN_NVS_W(TAG);
    storage->set("otaState", (int)otaState);
    storage->close();
    esp_ota_mark_app_invalid_rollback_and_reboot();
}

esp_err_t BootConfig_t::SetConfigurationParameters(const json &config_in)
{
    esp_err_t ret = ESP_FAIL;
    if (config_in[TAG].contains("params") != 0)
    {
        auto &bootconfig = config_in[TAG]["params"];
        AssertjsonInt(bootconfig, MAX_RESET_FACTORY, MaxRebootCountTriggerFactory, 0, 4095);
        if (isConfigured == true)
        {
            return SaveToNVS();
        }
        return ret;
    }
    return ret;
}

esp_err_t BootConfig_t::GetConfiguration(json &config_out) const
{
    try
    {
        config_out[TAG]["params"] =
            {
                {"MaxRestFac", MaxRebootCountTriggerFactory}};
        return ESP_OK;
    }
    catch (const std::exception &e)
    {
        ESP_LOGE(TAG, "%s", e.what());
        return ESP_FAIL;
    }
}

esp_err_t BootConfig_t::GetConfigurationStatus(json &config_out) const
{
    try
    {
        config_out[TAG]["status"] =
            {
                {IS_NEED_VALIDATION, IsNeedValidation()},
                {PS_RAM_OK, psramFound()},
                {IS_ROLLED_BACK, IsFactoryRolledBack() || IsOTARolledBack()},
                {LAST_STATUS, lastStatus},
                {OTA_STATUS, GetOtaStatusString(otaState)},
                {REBOOT_COUNTER, rebootCounter},
                {BOOT_REASON, static_cast<int>(bootReason)},
                {"isConfigured", isConfigured}};
        return ESP_OK;
    }
    catch (const std::exception &e)
    {
        ESP_LOGE(TAG, "%s", e.what());
        return ESP_FAIL;
    }
}

esp_err_t BootConfig_t::RestoreDefault()
{
    MaxRebootCountTriggerFactory = CONFIG_BOOT_DEFAULT_NUM_REBOOT_FACTORY_RESTORE;
    rebootCounter = 0;
    bootReason = NO_MEAN;
    otaState = ESP_OTA_IMG_UNDEFINED;
    lastStatus = NORMAL;
    SaveToNVS();
    return ESP_OK;
}

esp_err_t BootConfig_t::LoadFromNVS()
{
    NvsPointer storage = OPEN_NVS(TAG);
    esp_err_t ret = ESP_OK;
    ret |= storage->get(MAX_RESET_FACTORY, (MaxRebootCountTriggerFactory));
    int _BootReason = 0;
    ret |= storage->get(BOOT_REASON, (_BootReason));
    bootReason = static_cast<RESET_REASON>(_BootReason);
    ret |= storage->get(REBOOT_COUNTER, rebootCounter);
    int _otaState = 0;
    ret |= storage->get(OTA_STATUS, _otaState);
    otaState = static_cast<esp_ota_img_states_t>(_otaState);
    int _LastStatus;
    ret |= storage->get(LAST_STATUS, _LastStatus);
    lastStatus = static_cast<AladinStatus_t>(_LastStatus);
    return ret;
}

esp_err_t BootConfig_t::SaveToNVS()
{
    NvsPointer storage = OPEN_NVS_W(TAG);
    esp_err_t ret = ESP_OK;
    ret |= storage->set(MAX_RESET_FACTORY, (MaxRebootCountTriggerFactory));
    ret |= storage->set(BOOT_REASON, static_cast<int>(bootReason));
    ret |= storage->set(REBOOT_COUNTER, rebootCounter);
    ret |= storage->set(OTA_STATUS, static_cast<int>(otaState));
    ret |= storage->set(LAST_STATUS, static_cast<int>(lastStatus));
    return ret;
}

esp_err_t BootConfig_t::MqttCommandCallBack(const json &commandIn)
{
    esp_err_t ret = ESP_FAIL;
    if (commandIn[TAG].contains("command") != 0)
    {
        auto rgbCommand = commandIn[TAG]["command"];
        bool Found = false;
        std::string password;
        AssertjsonStr(rgbCommand, "ResetFactory", password);
        if (password == "I am Very Sure")
        {
            return RestoreFactory();
        }
        AssertjsonBool(rgbCommand, "RollbackOTA", Found);
        if (Found)
        {
            SetInvalidOtaImageAndReboot();
            return ESP_OK;
        }
        AssertjsonBool(rgbCommand, "ConfirmOTA", Found);
        if (Found)
        {
            return SetValidOtaImage();
        }
        AssertjsonBool(rgbCommand, "ResetRebootCounter", Found);
        if (Found)
        {
            rebootCounter = 0;
            NvsPointer storage = OPEN_NVS_W(TAG);
            storage->set("rebootCounter", rebootCounter);
            return ESP_OK;
        }
    }
    return ret;
}

/**
 * @brief Get the Reset Reason String object
 *
 * @param reason rebboot reason
 * @return std::string
 */
const std::string BootConfig_t::GetResetReasonString(const RESET_REASON &reason) const
{
    switch (reason)
    {
    case 1:
        return std::string("POWERON_RESET");
        break; /**<1,  Vbat power on reset*/
    case 3:
        return std::string("SW_RESET");
        break; /**<3,  Software reset digital core*/
    case 4:
        return std::string("OWDT_RESET");
        break; /**<4,  Legacy watch dog reset digital core*/
    case 5:
        return std::string("DEEPSLEEP_RESET");
        break; /**<5,  Deep Sleep reset digital core*/
    case 6:
        return std::string("SDIO_RESET");
        break; /**<6,  Reset by SLC module, reset digital core*/
    case 7:
        return std::string("TG0WDT_SYS_RESET");
        break; /**<7,  Timer Group0 Watch dog reset digital core*/
    case 8:
        return std::string("TG1WDT_SYS_RESET");
        break; /**<8,  Timer Group1 Watch dog reset digital core*/
    case 9:
        return std::string("RTCWDT_SYS_RESET");
        break; /**<9,  RTC Watch dog Reset digital core*/
    case 10:
        return std::string("INTRUSION_RESET");
        break; /**<10, Instrusion tested to reset CPU*/
    case 11:
        return std::string("TGWDT_CPU_RESET");
        break; /**<11, Time Group reset CPU*/
    case 12:
        return std::string("SW_CPU_RESET");
        break; /**<12, Software reset CPU*/
    case 13:
        return std::string("RTCWDT_CPU_RESET");
        break; /**<13, RTC Watch dog Reset CPU*/
    case 14:
        return std::string("EXT_CPU_RESET");
        break; /**<14, for APP CPU, reseted by PRO CPU*/
    case 15:
        return std::string("RTCWDT_BROWN_OUT_RESET");
        break; /**<15, Reset when the vdd voltage is not stable*/
    case 16:
        return std::string("RTCWDT_RTC_RESET");
        break; /**<16, RTC Watch dog reset digital core and rtc module*/
    default:
        return std::string("NO_MEAN");
    }
}

const std::string BootConfig_t::GetOtaStatusString(const esp_ota_img_states_t &reason) const
{
    switch (reason)
    {
    case ESP_OTA_IMG_NEW:
        return std::string("ESP_OTA_IMG_NEW");
        break; /**<1,  Vbat power on reset*/
    case ESP_OTA_IMG_PENDING_VERIFY:
        return std::string("ESP_OTA_IMG_PENDING_VERIFY");
        break; /**<3,  Software reset digital core*/
    case ESP_OTA_IMG_VALID:
        return std::string("ESP_OTA_IMG_VALID");
        break; /**<4,  Legacy watch dog reset digital core*/
    case ESP_OTA_IMG_INVALID:
        return std::string("ESP_OTA_IMG_INVALID");
        break; /**<5,  Deep Sleep reset digital core*/
    case ESP_OTA_IMG_ABORTED:
        return std::string("ESP_OTA_IMG_ABORTED");
        break; /**<6,  Reset by SLC module, reset digital core*/
    default:
        return std::string("ESP_OTA_IMG_UNDEFINED");
    }
}

BootConfig_p_t bootConfig;