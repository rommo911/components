#ifndef __PIR_H__
#define __PIR_H__
#include "task_tools.h"
#include "driver/gpio.h"
#include "freertos/event_groups.h"
#include "system.hpp"

class PIR : public Task
{
    private:
    bool status;
    std::chrono::seconds timeOut = 30s;
    unsigned long PIRTimeout;
    EventGroupHandle_t events;
    gpio_num_t pin;
    static const uint16_t ISR_EVENT = BIT(5);
    static IRAM_ATTR void ISR_Handler(void* arg);
    const char* TAG;
    EventLoop_p_t loop;
    public:
    ~PIR();
    PIR(EventLoop_p_t& _loop, gpio_num_t, gpio_mode_t);
    void run(TimerHandle_t arg);
    bool PIRLastStatus;
    void setup(gpio_num_t, gpio_mode_t mode);
    bool Status();
    bool GetVal() { return status; }
    static const uint16_t Changed_EVENT = BIT(5);
    void SetTimeout(const std::chrono::seconds& s)
    {
        timeOut = s;
    }
    const Event_t EVENT_PIR_OFF = { TAG, EventID_t(0) };
    const Event_t EVENT_PIR_ON = { TAG, EventID_t(1) };
    const Event_t EVENT_PIR_CHANGED = { TAG, EventID_t(2) };
    const Event_t EVENT_PIR_DUMMY_ISR = { TAG, EventID_t(3) };
    const Event_t EVENT_PIR_PIN_ON = { TAG, EventID_t(4) };
    const Event_t EVENT_PIR_PIN_OFF = { TAG, EventID_t(5) };

};

class SWITCH : public Task
{
    private:
    volatile bool status, lastStatus;
    volatile EventGroupHandle_t SWITCH_events;
    gpio_num_t pin;
    const EventBits_t ISR_EVENT = BIT(0);
    const EventBits_t Changed_EVENT = BIT(2);
    EventLoop_p_t loop;
    static IRAM_ATTR void ISR_Handler(void* arg);
    void run(void* arg);
    public:
    ~SWITCH();
    SWITCH(EventLoop_p_t&, gpio_num_t, gpio_mode_t);
    void Setup(gpio_num_t, gpio_mode_t);
    bool Status();
};

static constexpr esp_event_base_t SW_EVENT = ("SWITCH");
static const Event_t EVENT_SW_OFF(SW_EVENT, EventID_t(0));
static const Event_t EVENT_SW_ON(SW_EVENT, EventID_t(1));
static const Event_t EVENT_SW_CHANGED(SW_EVENT, EventID_t(2));

#endif // __PIR_H__