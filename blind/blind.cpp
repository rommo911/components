#include "blind.h"
#include "driver/gpio.h"
#include <string>
#include <cstdlib>
#include "nvs_tools.h"

Blind::Blind(homeassistant::BaseDevCtx& hass_device, EventLoop_p_t& _loop,
    gpio_num_t _pin_up,
    gpio_num_t _pin_down
) : Config(TAG), loop(_loop),
pin_up(_pin_up),
pin_down(_pin_down)
{
    timer = std::make_unique<ESPTimer>([this](void* arg) {TimerExecute(); }, "blind");
    InitThisConfig();
    gpio_pad_select_gpio(_pin_up);
    gpio_pad_select_gpio(pin_down);
    gpio_set_direction(pin_up, GPIO_MODE_OUTPUT);
    gpio_set_direction(pin_down, GPIO_MODE_OUTPUT);

    gpio_set_level(pin_up, 0);
    gpio_set_level(pin_down, 0);
    if (this->isInverted)
        std::swap(pin_up, pin_down);
    HassBlindDescovery = std::make_unique <homeassistant::BlindDiscovery>(hass_device, homeassistant::BlindDiscovery::window);
}

Blind::~Blind()
{
    gpio_set_level(pin_up, 0);
    gpio_set_level(pin_down, 0);
}

bool Blind::handle_set_per(const std::string& str1)
{
    uint8_t val = std::atoi(str1.c_str());
    try
    {
        bool ret = handle_set_per_uint(val);
        return ret;
    }
    catch (const std::exception& e)
    {
        std::cout << e.what() << '\n';
    }
    return false;
}

bool Blind::IsBusy() const
{
    return isBusy;
}

bool Blind::handle_set_per_uint(uint8_t New_percentage)
{
    if (New_percentage <= 100)
    {
        if (New_percentage == 0 && perc <= 5)
        {
            perc = 75;
        }
        if (New_percentage == 100 && perc >= 98)
        {
            perc = 25;
        }
        if (perc == New_percentage)
        {
            movingDirection = Direction_Stop;
            return true;
        }
        else
        {
            isBusy = true;
            loop->post_event(EVENT_BLIND_MOVING);
            if (perc > New_percentage)
            {
                movingDirection = Direction_Down;
                perc = New_percentage;
                timer->start_periodic(std::chrono::milliseconds(DownTime / 100), this);
            }
            else
            {
                movingDirection = Direction_UP;
                perc = New_percentage;
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
        isBusy = true;
        commandMode = true;
        loop->post_event(EVENT_BLIND_MOVING);
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
    perc = last_perc;
}

static  int throttle = 0;

void Blind::TimerExecute()
{
    if (movingDirection == Direction_Stop)
    {

        gpio_set_level(pin_up, 0);
        gpio_set_level(pin_down, 0);
        movingDirection = Direction_Stop;
        loop->post_event(EVENT_BLIND_STOPPED);
        throttle = 0;
        isBusy = false;
        commandMode = false;
        ESP_LOGI(TAG, "STOP");
        SaveToNVS();
        timer->stop();
        return;
    }
    else if (movingDirection == Direction_UP)
    {
        gpio_set_level(pin_up, 1);
        gpio_set_level(pin_down, 0);
        if (last_perc < 100)
        {
            last_perc++;
            //ESP_LOGI(TAG, "UP %d", last_perc);
            if (last_perc == perc && !commandMode)
            {
                movingDirection = Direction_Stop;
            }
        }
        else
        {
            movingDirection = Direction_Stop;
        }
    }
    else if (movingDirection == Direction_Down)
    {
        gpio_set_level(pin_up, 0);
        gpio_set_level(pin_down, 1);
        if (last_perc > 0)
        {
            last_perc--;
            if (last_perc == perc && !commandMode)
            {
                movingDirection = Direction_Stop;
            }
        }
        else
        {
            movingDirection = Direction_Stop;
        }
    }
    loop->post_event(EVENT_BLIND_CHNAGED, std::chrono::milliseconds(20));
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
Blind* blind = nullptr;