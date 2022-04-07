#include "vswitch.h"
#include "driver/gpio.h"
#include <string>
#include <cstdlib>
#include "nvs_tools.h"

vswitch_t::vswitch_t(const std::string& _name) : Config(TAG), name(_name)
{
    InitThisConfig();
}

vswitch_t::~vswitch_t()
{
    this->DettachThisConfig();
}


void vswitch_t::Set(bool val)
{
    this->status = val;
}


bool vswitch_t::Get()
{
    return this->status;
}


bool vswitch_t::handle_set_CMD(std::string& str)
{
    return false;
}

std::string vswitch_t::GetStatusStr()const
{
    std::string vswitch_tstate = "N/A";
    return vswitch_tstate;
}


esp_err_t vswitch_t::SetConfigurationParameters(const json& config_in)
{
    esp_err_t ret = ESP_FAIL;
    if (config_in.contains(TAG) != 0)
    {
        if (config_in[TAG].contains("params") != 0)
        {
            const auto& cfg = config_in[TAG]["params"];
            AssertjsonBool(cfg, "status", status);
            if (isConfigured)
            {
                return SaveToNVS();
            }
            return ret;
        }
    }
    return ret;
}

esp_err_t vswitch_t::SaveToNVS()
{
    auto nvs = OPEN_NVS_W(this->TAG);
    esp_err_t ret = ESP_FAIL;
    if (nvs->isOpen())
    {
        ret = nvs->set("status", status);
    }
    return ret;
}


esp_err_t vswitch_t::LoadFromNVS()
{
    auto nvs = OPEN_NVS(this->TAG);
    esp_err_t ret = ESP_FAIL;
    if (nvs->isOpen())
    {
        ret = nvs->get("status", status);
    }
    return ret;
}
//CONFIG OVERRIDE
esp_err_t vswitch_t::GetConfiguration(json& config_out) const
{
    try
    {
        config_out[TAG]["params"] =
        { {"status", status}, };
        return ESP_OK;
    }
    catch (const std::exception& e)
    {
        // ESP_LOGE(TAG, "%s", e.what());
        return ESP_FAIL;
    }
}

esp_err_t vswitch_t::RestoreDefault()
{
    status = false;
    return SaveToNVS();
}


esp_err_t vswitch_t::MqttCommandCallBack(const json& config_in)
{
    return ESP_OK;
}
