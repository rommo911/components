
//#include "Button2.h" //  https://github.com/LennartHennigs/Button2
#include "rotary.h"
#include "FreeRTOS.hpp"
#include "esp_log.h"
#define EV_QUEUE_LEN 5

Rotary::Rotary(std::shared_ptr<ESPEventLoop>& _loop,
    gpio_num_t rot_pin1,
    gpio_num_t rot_pin2,
    gpio_num_t pin1,
    const char* tag,
    int pos,
    uint16_t _min,
    uint16_t _max) : Task(tag), TAG(tag), loop(_loop)
{
    setup(rot_pin1, rot_pin2, pin1, pos, _min, _max);
}

Rotary::~Rotary()
{
}

void Rotary::ResetRotary(uint8_t rot)
{
    if (rot <= max && rot >= min)
    {
        //ESP_LOGI("rotary", "reset to %d", rot);
        Rotaryposition = rot;
    }
}

void Rotary::setup(gpio_num_t rot_pin1, gpio_num_t rot_pin2, gpio_num_t pin1, int pos, uint16_t _min, uint16_t _max)
{
    event_queue = xQueueCreate(EV_QUEUE_LEN, sizeof(rotary_encoder_event_t));
    min = _min;
    max = _max;
    rotary_encoder_init(event_queue, 1000);
    memset(&re, 0, sizeof(rotary_encoder_t));
    re.pin_a = rot_pin1;
    re.pin_b = rot_pin2;
    re.pin_btn = pin1;
    re.suspended = false;
    rotary_encoder_add(&re);
    Rotarychanged = false;
    Rotaryposition = pos;
    sleepTimer = std::make_unique<ESPTimer>([this](void* arg) {rotary_encoder_suspend(&re);}, "suspend rotary");
    StartTask(this);
}

void Rotary::run(void* arg)
{
    if (arg != NULL)
    {
        Rotary* _this = (Rotary*)arg;
        while (1)
        {
            uint32_t timout = 0;
            ESP_LOGI("rot", "Initial volume level: %d\n", _this->Rotaryposition);
            unsigned int queuTimeout = 500;
            while (1)
            {
                rotary_encoder_event_t e;
                if (xQueueReceive(event_queue, &e, pdMS_TO_TICKS(queuTimeout)) == pdTRUE)
                {
                    //ESP_LOGI("rot", "Got encoder event. type = %d, sender = 0x%08x, diff = %d\n", e.type, (uint32_t)e.sender, e.diff);
                    queuTimeout = 500;
                    switch (e.type)
                    {
                    case RE_ET_BTN_PRESSED:
                        //ESP_LOGI("rot", "Button pressed");
                        loop->post_event(EVENT_ROTARY_PRESSED);
                        break;
                    case RE_ET_BTN_RELEASED:
                        // ESP_LOGI("rot", "Button released");
                        loop->post_event(EVENT_ROTARY_BTN_RELEASEDT);
                        break;
                    case RE_ET_BTN_CLICKED:
                        // ESP_LOGI("rot", "Button clicked");
                        loop->post_event(EVENT_ROTARY_BTN_CLICKED);
                        break;
                    case RE_ET_BTN_LONG_PRESSED:
                        // ESP_LOGI("rot", "Looooong pressed button");
                        loop->post_event(EVENT_ROTARY_BTN_LONG_PRESSED);
                        break;
                    case RE_ET_CHANGED:
                        if (e.diff > 0 && (_this->Rotaryposition + e.diff) <= max)
                        {
                            timout = Task::Millis() + 1500;
                            _this->Rotaryposition += (e.diff * 2);
                            if (_this->Rotaryposition > max)
                                _this->Rotaryposition = max;
                            _this->Rotarychanged = true;
                            loop->post_event(EVENT_ROTARY_INC);
                            loop->post_event(EVENT_ROTARY_CHANGED);
                            // ESP_LOGI("rot", "Volume+ %d %d", _this->Rotaryposition, e.diff);
                        }
                        else if (e.diff < 0 && (_this->Rotaryposition + e.diff) >= min)
                        {
                            timout = Task::Millis() + 1500;
                            _this->Rotaryposition += (e.diff * 2);
                            if (_this->Rotaryposition < min)
                                _this->Rotaryposition = min;
                            _this->Rotarychanged = true;

                            loop->post_event(EVENT_ROTARY_DEC);
                            loop->post_event(EVENT_ROTARY_CHANGED);
                            // ESP_LOGI("rot", "Volume- %d %d", _this->Rotaryposition, e.diff);
                        }
                        break;
                    case RE_ET_WAKE:
                        rotary_encoder_resume(&re);
                        xQueueReset(event_queue);
                    default:
                        break;
                    }
                }
                else if (Task::Millis() > timout && _this->Rotarychanged)
                {
                    _this->Rotarychanged = false;
                    //ESP_LOGI("rot", "Volume timeout ");
                    loop->post_event(EVENT_ROTARY_TIMED_OUT);
                    queuTimeout = portMAX_DELAY;
                    sleepTimer->setPeriod(10s);
                    sleepTimer->reset_once();
                }
            }
        }
    }
}

uint8_t Rotary::GetPos()
{
    return Rotaryposition;
}

bool Rotary::isChanged()
{
    return Rotarychanged;
}
Rotary* rot = nullptr;