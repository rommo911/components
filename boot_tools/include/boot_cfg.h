#ifndef __BOOT_CFG_H__
#define __BOOT_CFG_H__
#pragma once

#include "config.hpp"
#include "esp32/rom/rtc.h"
#include "esp_attr.h"
#include "esp_ota_ops.h"
class BootConfig_t 
{
protected:
    static constexpr char TAG[] = "Boot";

public:
    BootConfig_t()
    {
    }
    ~BootConfig_t(){};
    // run boot sequence check
    esp_err_t Boot();

    // return true if OTA is performed and waiting for confirmation
    bool IsNeedValidation() const;
    // return true if OTA has failed and system rolled back
    bool IsOTARolledBack() const;
    //
    bool IsFactoryRolledBack() const;

    esp_err_t EraseInvalidApp();

    // validate the OTA image if pending confirmation
    esp_err_t SetValidOtaImage();
    // set OTA image is invalid and rollback and restart
    void SetInvalidOtaImageAndReboot(); // no return
    // reset to Factory app partition and reboot
    esp_err_t RestoreFactory(); // no return
    // get system reset / startup Reason in string
    const auto &GetRebootCount()
    {
        return rebootCounter;
    }
private:

    const std::string GetOtaStatusString(const esp_ota_img_states_t &reason) const;
    const std::string GetResetReasonString(const RESET_REASON &reason) const;
    esp_err_t CheckCurrentOTAPartitionStatus() const;
    bool CheckInvalidOtaState() const;
    const std::list<RESET_REASON> ErrorReason{TG0WDT_SYS_RESET, TG1WDT_SYS_RESET, RTCWDT_SYS_RESET, TGWDT_CPU_RESET, SW_CPU_RESET, RTCWDT_CPU_RESET, RTCWDT_RTC_RESET, RTCWDT_BROWN_OUT_RESET, EXT_CPU_RESET};
    RESET_REASON bootReason{NO_MEAN};
    esp_ota_img_states_t mutable otaState{ESP_OTA_IMG_UNDEFINED};
    uint8_t rebootCounter;
    uint8_t MaxRebootCountTriggerFactory = CONFIG_BOOT_DEFAULT_NUM_REBOOT_FACTORY_RESTORE;
    mutable esp_partition_t *runningPartition{nullptr};
    esp_err_t Diagnose()
    {
        return ESP_OK;
    }
};

/////////
typedef std::unique_ptr<BootConfig_t> BootConfig_p_t;
extern BootConfig_p_t bootConfig;
#endif // __BOOT_CFG_H__