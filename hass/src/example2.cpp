#include "homeassistant.h"
#include "esp_log.h"
#include "nlohmann/json.hpp"
#include "esp_mac.h"
#include "gpio.h"
class PIRSensor
{
private:
    gpio_num_t pin;
public:
    PIRSensor(int pinNumber, gpio_mode_t mode = GPIO_MODE_INPUT) :pin(static_cast<gpio_num_t> (pinNumber))
    {
        gpio_pad_select_gpio(pin);
        gpio_set_direction(static_cast<gpio_num_t>(pin), mode);
    }
    bool get_status()
    {
        return static_cast<bool>(gpio_get_level(pin));
    }
};
class SwitchSensor
{
private:
    /* data */
    gpio_num_t pin;
public:
    SwitchSensor(int pinNumber, gpio_mode_t mode = GPIO_MODE_INPUT) : pin(static_cast<gpio_num_t> (pinNumber))
    {
        gpio_pad_select_gpio(pin);
        gpio_set_direction(static_cast<gpio_num_t>(pin), mode);
    }
    bool get_status()
    {
        return static_cast<bool>(gpio_get_level(pin));
    }
};
class HassPIRSensor : private homeassistant::BinarySensorDiscovery, public PIRSensor
{
public:
    template<typename... Args>
    HassPIRSensor(homeassistant::BaseDevCtx& hass_device, Args&&... args) :

        homeassistant::BinarySensorDiscovery(hass_device, homeassistant::BinarySensorDiscovery::motion),
        PIRSensor(args...)
    {
    }
    ~HassPIRSensor();
    const std::pair<std::string, std::string> GenerateHassStatus()
    {
        nlohmann::json hassJson = { {this->GetClass().c_str(),PIRSensor::get_status()} };

        return std::make_pair(this->StatusTopic(), hassJson.dump());
    }
    const std::pair<std::string, std::string> GenerateHassdiscoveryMessage()
    {
        return std::make_pair(this->DiscoveryTopic(), this->DiscoveryMessage());
    }
};

class HassSwitchSensor : private homeassistant::BinarySensorDiscovery, public PIRSensor
{
public:
    template<typename... Args>
    HassSwitchSensor(homeassistant::BaseDevCtx& hass_device, Args&&... args) :

        homeassistant::BinarySensorDiscovery(hass_device, homeassistant::BinarySensorDiscovery::garage_door),
        PIRSensor(args...)
    {
    }
    ~HassSwitchSensor();
    const std::pair<std::string, std::string> GenerateHassStatus()
    {
        nlohmann::json hassJson = { {this->GetClass().c_str(),PIRSensor::get_status()} };

        return std::make_pair(this->StatusTopic(), hassJson.dump());
    }
    const std::pair<std::string, std::string> GenerateHassdiscoveryMessage()
    {
        return std::make_pair(this->DiscoveryTopic(), this->DiscoveryMessage());
    }
};

void example(void) {
    ESP_LOGI("HASS_TAG", "Example");
    // Example of how to use the HASS library
    // This example will send a message to the HASS server
    homeassistant::Device_Description_t hassDevCtxDescription;
    uint8_t mac[6];
    esp_read_mac(mac, ESP_MAC_WIFI_STA);

    hassDevCtxDescription.name = "HASS_TAG";
    hassDevCtxDescription.room = "Home";
    hassDevCtxDescription.manufacturer = "moi";
    hassDevCtxDescription.model = "pirSensor";
    hassDevCtxDescription.version = "1.0";
    hassDevCtxDescription.name = std::string("ESP32-") + std::to_string(mac[5]);
    homeassistant::BaseDevCtx hassDevCtx(hassDevCtxDescription);
    ////////////////////
    ////////////////////
    const int pin = 22;
    HassPIRSensor hassTempSensor(hassDevCtx, pin);
    const auto hassdiscoveryMesasge = hassTempSensor.GenerateHassdiscoveryMessage(); // call this once mqtt is connected to trigger discovery in hass server 
    // mqtt.publish(hassdiscoveryMesasge.first, hassdiscoveryMesasge.second);
    const auto hassStatus = hassTempSensor.GenerateHassStatus();
    //mqtt.publish(hassStatus.first, hassStatus.second);

    ////////////////////
    ////////////////////
    const int pin2 = 23;
    HassSwitchSensor hassSwitchSensor(hassDevCtx, pin2);
    const auto hassdiscoveryMesasge2 = hassSwitchSensor.GenerateHassdiscoveryMessage(); // call this once mqtt is connected to trigger discovery in hass server 
    // mqtt.publish(hassdiscoveryMesasge.first, hassdiscoveryMesasge.second);
    const auto hassStatus2 = hassSwitchSensor.GenerateHassStatus();
    //mqtt.publish(hassStatus.first, hassStatus.second);


}