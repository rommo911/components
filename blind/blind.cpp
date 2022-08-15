#include "blind.h"
#include "driver/gpio.h"
#include <string>
#include <cstdlib>
#include "nvs_tools.h"
xQueueHandle Blind::InterruptQueue = NULL;

Blind::Blind(EventLoop_p_t& _loop,
    gpio_num_t _pin_up,
    gpio_num_t _pin_down
) : Config(TAG), loop(_loop),
pin_up(_pin_up),
pin_down(_pin_down)
{
    timer = std::make_unique<ESPTimer>([this](void* arg) {TimerExecute(); }, "blind");
    InitThisConfig();
    if (this->isInverted)
        std::swap(pin_up, pin_down);
    timerIntr = std::make_unique<ESPTimer>([this](void* arg) { TimerExecuteIntr(); }, "blind2");
    InterruptQueue = xQueueCreate(10, sizeof(uint32_t));
    Intertask = new AsyncTask([this](void* arg) {this->InterruptTask(); }, "blind interrupt", (void*)this);
    EnableInterrupt(true);
}

Blind::~Blind()
{
    EnableInterrupt(false);
    gpio_set_level(pin_up, 0);
    gpio_set_level(pin_down, 0);
}

bool Blind::handle_set_per(const std::string& str1)
{
    uint8_t val = std::atoi(str1.c_str());
    return handle_set_per_uint(val);
}

bool Blind::IsBusy() const
{
    return isBusy;
}

bool Blind::handle_set_per_uint(uint8_t New_percentage)
{
    if (New_percentage <= 100)
    {
        uint8_t New_percentage__ = New_percentage;
        if (New_percentage == 0 && perc <= 5)
        {
            New_percentage__ = 30;
        }
        else  if (New_percentage == 100 && perc >= 90)
        {
            New_percentage__ = 60;
        }
        if (perc == New_percentage)
        {
            movingDirection = Direction_Stop;
            return true;
        }
        else
        {
            EnableInterrupt(false);
            if (perc > New_percentage)
            {
                movingDirection = Direction_Down;
                perc = New_percentage__;
                timer->start_periodic(std::chrono::milliseconds(DownTime / 100), this);
            }
            else
            {
                movingDirection = Direction_UP;
                perc = New_percentage__;
                timer->start_periodic(std::chrono::milliseconds(UpTime / 100), this);
            }
        }
    }
    return false;
}

bool Blind::handle_set_CMD(MovingDirection_t direction)
{
    movingDirection = direction;
    if (direction != Direction_Stop)
    {
        commandMode = true;
        EnableInterrupt(false);
        if (direction == Direction_Down)
        {
            timer->start_periodic(std::chrono::milliseconds(DownTime / 100));
        }
        if (direction == Direction_UP)
        {
            timer->start_periodic(std::chrono::milliseconds(UpTime / 100));
        }
    }
    return false;
}

uint8_t Blind::GetCurrentPerc() const
{
    return last_perc;
}

uint8_t Blind::GetTargetPerc()const
{
    return perc;
}

void Blind::Cancel()
{
    movingDirection = Direction_Stop;
    gpio_set_level(pin_up, 0);
    gpio_set_level(pin_down, 0);
    EnableInterrupt(true);
    perc = last_perc;
}

static  int throttle = 0;

void Blind::TimerExecute()
{
    switch (movingDirection)
    {
    case Direction_Stop:
    {
        gpio_set_level(pin_up, 0);
        gpio_set_level(pin_down, 0);
        movingDirection = Direction_Stop;
        loop->post_event(EVENT_BLIND_STOPPED);
        throttle = 0;
        isBusy = false;
        commandMode = false;
        perc = last_perc;
        ESP_LOGI(TAG, "STOP");
        SaveToNVS();
        timer->stop();
        EnableInterrupt(true);
        break;
    }
    case Direction_UP:
    {
        ESP_LOGI(TAG, "UP %d %d-command %d", last_perc, perc, commandMode);
        isBusy = true;
        gpio_set_level(pin_up, 1);
        gpio_set_level(pin_down, 0);
        if (last_perc < 100)
        {
            last_perc++;
            loop->post_event(EVENT_BLIND_CHNAGED, std::chrono::milliseconds(20));
            if (last_perc == perc && !commandMode)
            {
                movingDirection = Direction_Stop;
            }
        }
        else
        {
            movingDirection = Direction_Stop;
        }
        break;
    }
    case Direction_Down:
    {
        ESP_LOGI(TAG, "DOWN %d %d -command %d", last_perc, perc, commandMode);
        isBusy = true;
        gpio_set_level(pin_up, 0);
        gpio_set_level(pin_down, 1);
        if (last_perc > 0)
        {
            last_perc--;
            loop->post_event(EVENT_BLIND_CHNAGED, std::chrono::milliseconds(20));
            if (last_perc == perc && !commandMode)
            {
                movingDirection = Direction_Stop;
            }
        }
        else
        {
            movingDirection = Direction_Stop;
        }
        break;
    }
    }
    if (commandMode)
        perc = last_perc;
    return;
}

std::string Blind::GetStatusStr()const
{
    std::string blindstate = "N/A";
    if (IsBusy() == true)
    {
        if (movingDirection == Direction_Down)
        {
            blindstate = "closing";
        }
        else
        {
            blindstate = "opening";
        }
    }
    else
    {
        if (GetCurrentPerc() > 30)
            blindstate = "open";
        else
            blindstate = "closed";
    }
    return blindstate;
}


esp_err_t Blind::mqtt_callback(const std::string& topic, const std::string& data, void* arg)
{
    Blind* _this = (Blind*)arg;
    if ((topic.find("blind") != -1 || topic.find("window") != -1))
    {
        if ((topic.find("cmd") != -1 || topic.find("command") != -1))
        {
            if (data.find("OPEN") != -1)
            {
                return  _this->handle_set_CMD(Direction_UP) ? ESP_OK : ESP_FAIL;
            }
            if (data.find("CLOSE") != -1)
            {
                return _this->handle_set_CMD(Direction_Down) ? ESP_OK : ESP_FAIL;
            }
            if (data.find("STOP") != -1)
            {
                return  _this->handle_set_CMD(Direction_Stop) ? ESP_OK : ESP_FAIL;
            }
        }
        else
            if ((topic.find("setpos") != -1 || topic.find("set_pos") != -1 || topic.find("pos") != -1))
            {
                return  _this->handle_set_per(data) ? ESP_OK : ESP_FAIL;
            }
    }
    return ESP_FAIL;
}

esp_err_t Blind::SetConfigurationParameters(const json& config_in)
{
    esp_err_t ret = ESP_FAIL;
    if (config_in.contains(TAG) != 0)
    {
        if (config_in[TAG].contains("params") != 0)
        {
            const auto& mqttConfig = config_in[TAG]["params"];
            AssertjsonInt(mqttConfig, "position", last_perc, 0, 100);
            AssertjsonInt(mqttConfig, "TimerUP", UpTime, 10000, 30000);
            AssertjsonInt(mqttConfig, "TimerDown", DownTime, 10000, 30000);
            AssertjsonBool(mqttConfig, "isInverted", isInverted);
            if (isConfigured)
            {
                return SaveToNVS();
            }
            return ret;
        }
    }
    return ret;
}

esp_err_t Blind::SaveToNVS()
{
    auto nvs = OPEN_NVS_W(this->TAG);
    esp_err_t ret = ESP_FAIL;
    if (nvs->isOpen())
    {
        ret = nvs->set("position", last_perc);
        ret |= nvs->set("TimerUP", UpTime);
        ret |= nvs->set("TimerDown", DownTime);
        ret |= nvs->set("isInverted", isInverted);
    }
    return ret;
}
//CONFIG OVERRIDE
esp_err_t Blind::GetConfiguration(json& config_out) const
{
    try
    {
        config_out[TAG]["params"] =
        { {"position", last_perc},
         {"TimerUP", UpTime},
         {"TimerDown", DownTime},
         {"isInverted", isInverted}, };
        return ESP_OK;
    }
    catch (const std::exception& e)
    {
        ESP_LOGE(TAG, "%s", e.what());
        return ESP_FAIL;
    }
}

esp_err_t Blind::RestoreDefault()
{
    DownTime = 10000;
    UpTime = 10000;
    last_perc = 0;
    return ESP_OK;
}
esp_err_t Blind::LoadFromNVS()
{
    auto nvs = OPEN_NVS(this->TAG);
    esp_err_t ret = ESP_FAIL;
    if (nvs->isOpen())
    {
        ret = nvs->get("TimerUP", UpTime);
        ret |= nvs->get("TimerDown", DownTime);
        ret |= nvs->get("position", last_perc);
        ret |= nvs->get("isInverted", isInverted);
        if (last_perc > 100)
            last_perc = 100;
        perc = last_perc;
    }
    return ret;
}
void Blind::EnableInterrupt(const bool val)
{
    if (val)
    {
        gpio_install_isr_service(0);
        gpio_set_direction(pin_up, GPIO_MODE_INPUT);
        gpio_set_direction(pin_down, GPIO_MODE_INPUT);
        gpio_set_intr_type(pin_up, GPIO_INTR_ANYEDGE);
        gpio_set_intr_type(pin_down, GPIO_INTR_ANYEDGE);
        gpio_isr_handler_add(pin_up, Blind::up_isr_handler, static_cast<void*>(this));
        gpio_isr_handler_add(pin_down, Blind::down_isr_handler, static_cast<void*>(this));

    }
    else
    {
        gpio_isr_handler_remove(pin_up);
        gpio_isr_handler_remove(pin_down);
        gpio_set_direction(pin_up, GPIO_MODE_OUTPUT);
        gpio_set_direction(pin_down, GPIO_MODE_OUTPUT);
        gpio_set_level(pin_up, 0);
        gpio_set_level(pin_down, 0);
        timerIntr->stop();
    }
}

void Blind::TimerExecuteIntr()
{
    if (movingDirection == Direction_Stop)
    {
        throttle = 0;
        isBusy = false;
        SaveToNVS();
        perc = last_perc;
        loop->post_event(EVENT_BLIND_STOPPED);
        timerIntr->stop();
        ESP_LOGI(TAG, "STOP INTR");
        return;
    }
    else if (movingDirection == Direction_UP)
    {
        ESP_LOGI(TAG, "UP INTR");
        isBusy = true;
        if (last_perc < 100)
        {
            last_perc++;

        }
        else
        {
            movingDirection = Direction_Stop;
        }
    }
    else if (movingDirection == Direction_Down)
    {
        ESP_LOGI(TAG, "DOWN INTR");
        isBusy = true;
        if (last_perc > 0)
        {
            last_perc--;
        }
        else
        {
            movingDirection = Direction_Stop;
        }
    }
    loop->post_event(EVENT_BLIND_CHNAGED, std::chrono::milliseconds(20));

}

void Blind::InterruptTask()
{
    while (true)
    {
        InterruptState_t event;
        auto res = xQueueReceive(InterruptQueue, &event, portMAX_DELAY);
        if (res == pdTRUE)
        {
            switch ((event))
            {
            case InterruptState_t::Up:
            {
                movingDirection = MovingDirection_t::Direction_UP;
                timerIntr->start_periodic(std::chrono::milliseconds(UpTime / 100));
            }
            break;
            case InterruptState_t::Down:
            {
                movingDirection = MovingDirection_t::Direction_Down;
                timerIntr->start_periodic(std::chrono::milliseconds(UpTime / 100));
            }
            break;
            case InterruptState_t::Stop:
            {
                movingDirection = MovingDirection_t::Direction_Stop;
            }
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(30));
        //xQueueReset(InterruptQueue);
    }

}

void Blind::up_isr_handler(void* arg)
{
    Blind* _this = static_cast<Blind*>(arg);
    if (gpio_get_level(_this->pin_up) == 1)
    {
        _this->InterruptState = InterruptState_t::Up;
    }
    else
    {
        _this->InterruptState = InterruptState_t::Stop;
    }
    if (_this->InterruptQueue != NULL)
        xQueueSendFromISR(_this->InterruptQueue, &_this->InterruptState, NULL);

}

void Blind::down_isr_handler(void* arg)
{
    Blind* _this = static_cast<Blind*>(arg);
    if (gpio_get_level(_this->pin_down) == 1)
    {
        _this->InterruptState = InterruptState_t::Down;
    }
    else
    {
        _this->InterruptState = InterruptState_t::Stop;
    }
    if (_this->InterruptQueue != NULL)
        xQueueSendFromISR(_this->InterruptQueue, &_this->InterruptState, NULL);
}