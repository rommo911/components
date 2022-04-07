#ifndef __vswitch_t_H__
#define __vswitch_t__
#include "system.hpp"
#include <string>

#define DEFAULT_UP_DOWN_TIME_MS 15000

class vswitch_t :public Config
{
public:

    static constexpr char TAG[] = "vswitch";

    vswitch_t(const std::string& _name);
    ~vswitch_t();
    std::string name;
    bool handle_set_CMD(std::string& str);
    bool Get();
    void Set(bool val);

    std::string GetStatusStr()const;
    const Event_t EVENT_ = { TAG, EventID_t(0) };
    esp_err_t SetConfigurationParameters(const json& config_in) override;
    esp_err_t GetConfiguration(json& config_out) const override;
    esp_err_t SaveToNVS() override;
private:
    //CONFIG OVERRIDE
    esp_err_t RestoreDefault() override;
    esp_err_t LoadFromNVS() override;
    esp_err_t MqttCommandCallBack(const json& config_in)override;

    bool status = false;
};
#endif // __BLIND_H__