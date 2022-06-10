#include "homeassistant.h"
#include "esp_log.h"
#include "nlohmann/json.hpp"
 class tempSensor
    {
    private:
        /* data */
    public:
        tempSensor(int addr)
        {
            //initSEnsor(addr);
        }
        ~tempSensor();
        int get_temp()
        {
            return 22;
        }
    };
   class HassTempSensor : private homeassistant::SensorDiscovery, public tempSensor
    {
    public:
    template<typename... Args>
        HassTempSensor(homeassistant::BaseDevCtx& hass_device, Args&&... args) :
                                   
                                    homeassistant::SensorDiscovery(hass_device, homeassistant::SensorDiscovery::temperature, "C"),
                                    tempSensor(args...)
        {
        }
        ~HassTempSensor();
        const std::pair<std::string, std::string> GenerateHassStatus()
        {
            nlohmann::json hassJson = { {this->GetClass().c_str(),tempSensor::get_temp()} };
            return std::make_pair(this->StatusTopic(), hassJson.dump());
        }
        const std::pair<std::string, std::string> GenerateHassdiscoveryMessage()
        {
         return std::make_pair(this->DiscoveryTopic(),this->DiscoveryMessage());
        }
    }; 

void example (void) {
    ESP_LOGI("HASS_TAG", "Example");

    // Example of how to use the HASS library
    // This example will send a message to the HASS server
    homeassistant::Device_Description_t hassDevCtxDescription;
    hassDevCtxDescription.name = "HASS_TAG";
    hassDevCtxDescription.room = "Home";
    hassDevCtxDescription.manufacturer = "ESP32";
    hassDevCtxDescription.model = "ESP32";
    hassDevCtxDescription.version = "1.0";
    hassDevCtxDescription.name = "ESP32";
    homeassistant::BaseDevCtx hassDevCtx(hassDevCtxDescription);
    HassTempSensor hassTempSensor(hassDevCtx,0x66); 
    const auto hassdiscoveryMesasge = hassTempSensor.GenerateHassdiscoveryMessage();
     // mqtt.publish(hassdiscoveryMesasge.first, hassdiscoveryMesasge.second);
    const auto hassStatus = hassTempSensor.GenerateHassStatus();
    //mqtt.publish(hassStatus.first, hassStatus.second);


}