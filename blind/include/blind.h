#ifndef __BLIND_H__
#define __BLIND_H__
#include "task_tools.h"
#include "system.hpp"
#include "driver/gpio.h"
#include "freertos/event_groups.h"
#include <vector>
#include <string>
#include "FreeRTOS.hpp"
#include "homeassistant.h"
#include <memory>

#define DEFAULT_UP_DOWN_TIME_MS 15000

class Blind :public Config
{
    public:
    enum MovingDirection_t
    {
        Direction_UP = 0,
        Direction_Down = 1,
        Direction_Stop = 2
    } movingDirection;
    static constexpr char TAG[] = "BLIND";
    int DownTime = DEFAULT_UP_DOWN_TIME_MS;
    int UpTime = DEFAULT_UP_DOWN_TIME_MS;
    Blind(EventLoop_p_t& _loop, gpio_num_t pin_up, gpio_num_t pin_down);
    ~Blind();
    bool handle_set_per(const std::string& str1);
    bool handle_set_per_uint(uint8_t percentage);
    bool handle_set_CMD(MovingDirection_t direction);
    uint8_t GetCurrentPerc()const;
    uint8_t GetTargetPerc()const;
    void Cancel();
    bool IsBusy()const;
    static esp_err_t mqtt_callback(const std::string&, const std::string&, void*);
    std::unique_ptr<ESPTimer> timer{ nullptr };
    std::string GetStatusStr()const;
    const Event_t EVENT_BLIND_MOVING = { TAG, EventID_t(0) };
    const Event_t EVENT_BLIND_STOPPED = { TAG, EventID_t(2) };
    const Event_t EVENT_BLIND_CHNAGED = { TAG, EventID_t(3) };
    esp_err_t SetConfigurationParameters(const json& config_in) override;
    esp_err_t GetConfiguration(json& config_out) const override;
    esp_err_t SaveToNVS() override;
    bool IsIverted() { return isInverted; }
    private:
    //CONFIG OVERRIDE
    esp_err_t RestoreDefault() override;
    esp_err_t LoadFromNVS() override;
    EventLoop_p_t loop{ nullptr };
    SemaphorePointer_t lock;
    gpio_num_t pin_up, pin_down;
    uint8_t last_perc = 0;
    uint8_t perc = 0;
    bool isBusy = false;
    bool commandMode = false;
    bool isInverted = false;
    void TimerExecute();
};
#endif // __BLIND_H__