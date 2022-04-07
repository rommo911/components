#ifndef __ROTARY_H__
#define __ROTARY_H__

#include <encoder.h>
#include <string.h>
#include "task_tools.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "system.hpp"

using namespace idf::event;
using namespace std;
#define RE_A_GPIO 16
#define RE_B_GPIO 17
#define RE_BTN_GPIO 5

class Rotary : public Task
{
    private:
    ESPTimer_p_t sleepTimer{ nullptr };
    const char* TAG;
    QueueHandle_t event_queue;
    rotary_encoder_t re;
    int16_t Rotaryposition;
    bool Rotarychanged;
    unsigned long rotary_timouet;
    uint16_t max;
    uint16_t min;
    void run(void*);
    std::shared_ptr<ESPEventLoop>& loop;

    public:
    Rotary(std::shared_ptr<ESPEventLoop>& _loop, const char* tag = "ROTARY") : Task(tag), TAG(tag), loop(_loop) {};
    Rotary(std::shared_ptr<ESPEventLoop>& _loop, gpio_num_t rot_pin1, gpio_num_t rot_pin2, gpio_num_t pin1, const char* tag = "rotary", int pos = 0, uint16_t _min = 0, uint16_t _max = 100);
    ~Rotary();
    void setup(gpio_num_t rot_pin1, gpio_num_t rot_pin2, gpio_num_t pin1, int pos = 0, uint16_t _min = 0, uint16_t _max = 100);
    void ResetRotary(uint8_t rot);
    uint8_t GetPos();
    bool isChanged();
    const Event_t EVENT_CHANGED_1_PERC = { TAG, EventID_t(0) };
    const Event_t EVENT_ROTARY_PRESSED = { TAG, EventID_t(1) };
    const Event_t EVENT_ROTARY_BTN_RELEASEDT = { TAG, EventID_t(2) };
    const Event_t EVENT_ROTARY_BTN_CLICKED = { TAG, EventID_t(3) };
    const Event_t EVENT_ROTARY_BTN_LONG_PRESSED = { TAG, EventID_t(4) };
    const Event_t EVENT_ROTARY_CHANGED = { TAG, EventID_t(5) };
    const Event_t EVENT_ROTARY_TIMED_OUT = { TAG, EventID_t(6) };
    const Event_t EVENT_ROTARY_INC = { TAG, EventID_t(7) };
    const Event_t EVENT_ROTARY_DEC = { TAG, EventID_t(8) };
};

extern Rotary* rot;
#endif // __ROTARY_H__