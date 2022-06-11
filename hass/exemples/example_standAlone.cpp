#include "homeassistant.h"
#include "esp_log.h"
#include "nlohmann/json.hpp"

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
    auto hassdiscoverySensorTemperature = new homeassistant::SensorDiscovery(hassDevCtx, homeassistant::SensorDiscovery::temperature, "C"); 
     // mqtt.publish(hassdiscoveryMesasge.first, hassdiscoveryMesasge.second);
    auto hassdiscoverySensorHumidity = new homeassistant::SensorDiscovery(hassDevCtx, homeassistant::SensorDiscovery::humidity, "%"); 
    //mqtt.publish(hassStatus.first, hassStatus.second);
    const std::string discoveryMessage = hassdiscoverySensorTemperature->DiscoveryMessage();
    const std::string discoveryTopic = hassdiscoverySensorTemperature->DiscoveryTopic();
    //mqtt.publish(discoveryTopic, discoveryMessage);
    nlohmann::json hassJson = { {hassdiscoverySensorTemperature->GetClass().c_str(),22} };
    const std::string statusTopic = hassdiscoverySensorTemperature->StatusTopic();
    const std::string statusMessage = hassJson.dump();
    //mqtt.publish(statusTopic, statusMessage);

}