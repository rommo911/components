/*

HOME ASSISTANT MODULE

Original module
Copyright (C) 2017-2019 by Xose Pérez <xose dot perez at gmail dot com>

Reworked queueing and RAM usage reduction
Copyright (C) 2019-2021 by Maxim Prokhorov <prokhorov dot max at outlook dot com>

*/
#include "homeassistant.h"
#include "esp_log.h"
namespace homeassistant {

    static constexpr char mqtt_payload_online[] = "online";
    static constexpr char mqtt_payload_offline[] = "offline";
    static constexpr char payload_on[] = "on";
    static constexpr char payload_off[] = "off";
    static constexpr char payload_toggle[] = "toggle";
    static constexpr char payload_up[] = "OPEN";
    static constexpr char payload_down[] = "CLOSE";
    static constexpr char payload_position[] = "set_position";
    static constexpr char payload_ping[] = "ping";
    static constexpr char mqtt_topic_connection[] = "connection";
    static constexpr char mqtt_topic_status[] = "status";

    enum class PayloadCommand_t {
        Off = 0,
        On = 1,
        Toggle = 2,
        open = 3,
        close = 4,
        SetPosition = 5,
        Ping = 6,
        Unknown = 0xFF
    };

    BaseDevCtx::BaseDevCtx(Device_Description_t des) :
        deviceDescription(des)
    {
        _json["dev"]["name"] = deviceDescription.name.c_str();
        _json["dev"]["mdl"] = deviceDescription.model.c_str();
        _json["dev"]["sw"] = deviceDescription.version.c_str();
        _json["dev"]["mf"] = deviceDescription.manufacturer.c_str();
        _json["room"] = deviceDescription.room.c_str();
        _json["dev"]["identifiers"] = { deviceDescription.MAC.c_str() };
    }
    void BaseDevCtx::SetDescription(Device_Description_t des)
    {
        deviceDescription = des;
        _json.clear();
        _json["dev"]["name"] = deviceDescription.name.c_str();
        _json["dev"]["mdl"] = deviceDescription.model.c_str();
        _json["dev"]["sw"] = deviceDescription.version.c_str();
        _json["dev"]["mf"] = deviceDescription.manufacturer.c_str();
        _json["room"] = deviceDescription.room.c_str();
        _json["dev"]["identifiers"] = { deviceDescription.MAC.c_str() };

    }
    const std::string& BaseDevCtx::name() const {
        return deviceDescription.name;
    }
    const std::string& BaseDevCtx::room() const {
        return deviceDescription.room;
    }
    const std::string& BaseDevCtx::MAC() const {
        return deviceDescription.MAC;
    }
    nlohmann::json BaseDevCtx::JsonObject() const
    {
        return _json;
    }

    // - Discovery object is expected to accept Context reference as input
    //   (and all implementations do just that)
    // - topic() & message() return refs, since those *may* be called multiple times before advancing to the next 'entity'
    // - We use short-hand names right away, since we don't expect this to be used to generate yaml
    // - In case the object uses the JSON makeObject() as state, make sure we don't use it (state)
    //   and the object itself after next() or ok() return false
    // - Make sure JSON state is not created on construction, but lazy-loaded as soon as it is needed.
    //   Meaning, we don't cause invalid refs immediatly when there are more than 1 discovery object present and we reset the storage.




    Discovery::Discovery(BaseDevCtx& ctx, const std::string& _hass_mqtt_device) :
        _BaseDevCtx(ctx),
        hass_mqtt_device(_hass_mqtt_device)
    {
        
    }
    void Discovery::ProcessJson()
    {
        discoveryJson = _BaseDevCtx.JsonObject();
        state_topic = "~/";
        command_topic = "~/";
        this->unique_id.clear();
        this->unique_id << _BaseDevCtx.room().c_str();
        unique_id << '_';
        unique_id << _BaseDevCtx.name().c_str();
        unique_id << "_";
        unique_id << _BaseDevCtx.MAC().c_str();
        unique_id << '_';
        topics_prefix.clear();
        this->topics_prefix << _BaseDevCtx.room().c_str();
        this->topics_prefix << "/";
        this->topics_prefix << _BaseDevCtx.name().c_str();
        this->topics_prefix << "_";
        this->topics_prefix << _BaseDevCtx.MAC().c_str();
        //
        this->availability_topic = topics_prefix.str().c_str();
        this->availability_topic += "/connection";
        //     
        discovery_topic.clear();
        this->discovery_topic << HOMEASSISTANT_PREFIX;
        this->discovery_topic << ("/");
        this->discovery_topic << hass_mqtt_device;
        this->discovery_topic << "/";

        this->discoveryJson["payload_available"] = mqtt_payload_online;
        this->discoveryJson["payload_not_available"] = mqtt_payload_offline;
        this->ProcessFinalJson();
        this->procced = true;
    }
    void Discovery::DumpDebugAll()
    {
        ESP_LOGW("HASS", " discovery_topic \n\r %s \n\r", discovery_topic.str().c_str());
        ESP_LOGW("HASS", " availability_topic \n\r %s \n\r", availability_topic.c_str());
        ESP_LOGW("HASS", "StatusTopic \n\r %s \n\r", StatusTopic().c_str());
        ESP_LOGW("HASS", " CommandTopic \n\r %s \n\r", CommandTopic().c_str());
        ESP_LOGW("HASS", " DiscoveryMessage \n\r %s \n\r", discovery_message.c_str());
    }
    const std::string& Discovery::ConnectionTopic()
    {
        return this->availability_topic;
    }
    const std::string Discovery::DiscoveryTopic() { return  this->discovery_topic.str(); }
    const std::string& Discovery::AvailabilityTopic() { return  this->availability_topic; }
    std::string Discovery::StatusTopic() { return std::string(topics_prefix.str() + "/state"); };
    std::string Discovery::CommandTopic() { return  std::string(topics_prefix.str() + "/cmd"); }
    const std::string& Discovery::DiscoveryMessage() { return  this->discovery_message; }

    void RelayDiscovery::ProcessFinalJson()
    {
        this->unique_id << _switch_name;
        //
        this->topics_prefix << "/" << _switch_name;

        std::string name = _BaseDevCtx.name() + "_" + _switch_name;
        this->state_topic += "state";
        //
        //
        this->discovery_topic << unique_id.str().c_str();
        this->discovery_topic << ("/config");
        //
        this->command_topic += "cmd";
        //
        this->discoveryJson["state_topic"] = state_topic.c_str();
        this->discoveryJson["unique_id"] = unique_id.str().c_str();
        this->discoveryJson["availability_topic"] = availability_topic.c_str();
        this->discoveryJson["~"] = topics_prefix.str().c_str();
        this->discoveryJson["pl_on"] = "ON";
        this->discoveryJson["pl_off"] = "OFF";
        this->discoveryJson["command_topic"] = command_topic;
        this->discoveryJson["name"] = name.c_str();
        this->discoveryJson["device_class"] = _class_type.c_str();
        this->discovery_message = discoveryJson.dump(5);
    }

    void BlindDiscovery::ProcessFinalJson()
    {
        //
        this->unique_id << _class_type;
        //
        this->positionTopic = state_topic;
        this->positionTopic += "position";
        this->state_topic += "state";
        //
        this->discovery_topic << unique_id.str().c_str();
        this->discovery_topic << ("/config");
        //
        this->topics_prefix << "/" << _class_type;
        //
        //
        this->discoveryJson["payload_open"] = "OPEN";
        this->discoveryJson["~"] = topics_prefix.str().c_str();
        this->discoveryJson["payload_close"] = "CLOSE";
        this->discoveryJson["availability_topic"] = availability_topic.c_str();
        this->setPosTopic = command_topic + "set_pos";
        this->discoveryJson["set_position_topic"] = setPosTopic.c_str();
        command_topic += "cmd";
        this->discoveryJson["command_topic"] = command_topic.c_str();
        this->discoveryJson["position_topic"] = positionTopic.c_str();
        this->discoveryJson["name"] = unique_id.str().c_str();
        this->discoveryJson["state_topic"] = state_topic.c_str();
        this->discoveryJson["payload_stop"] = "STOP";
        this->discoveryJson["state_open"] = "open";
        this->discoveryJson["state_opening"] = "opening";
        this->discoveryJson["state_closed"] = "closed";
        this->discoveryJson["state_closing"] = "closing";
        this->discoveryJson["position_template"] = "{{ value_json.position }}";
        this->discoveryJson["value_template"] = "{{ value_json.state }}";
        this->discoveryJson["device_class"] = _class_type;
        this->discoveryJson["unique_id"] = unique_id.str().c_str();
        this->discoveryJson["position_open"] = 100;
        this->discoveryJson["position_closed"] = 0;
        this->discovery_message = discoveryJson.dump(5);
    }

    void SensorDiscovery::ProcessFinalJson()
    {
        this->unique_id << _sensorClass;
        //
        this->state_topic += "state";
        //
        this->discovery_topic << unique_id.str().c_str() << ("/config");
        //
        this->topics_prefix << "/" << name.c_str();
        //
        this->discoveryJson["unique_id"] = unique_id.str().c_str();
        //
        this->discoveryJson["~"] = topics_prefix.str().c_str();
        this->discoveryJson["availability_topic"] = availability_topic.c_str();
        this->discoveryJson["state_topic"] = state_topic.c_str();
        std::string val = "{{ value_json.";
        val += _sensorClass;
        val += "}}";
        this->discoveryJson["value_template"] = val.c_str();
        if ((__unit) != "N/A")
        {
            this->discoveryJson["unit_of_meas"] = __unit.c_str();
        }
        this->discoveryJson["device_class"] = _sensorClass.c_str();
        this->discoveryJson["name"] = unique_id.str().c_str();
        this->discovery_message = discoveryJson.dump(5);

    }

    void BinarySensorDiscovery::ProcessFinalJson()
    {
        this->unique_id << _sensorClass;
        //
        this->state_topic += "state";
        //
        this->discovery_topic << unique_id.str().c_str() << ("/config");
        //
        this->topics_prefix << "/" << name.c_str();
        //
        this->discoveryJson["unique_id"] = unique_id.str().c_str();
        //
        this->discoveryJson["~"] = topics_prefix.str().c_str();
        this->discoveryJson["availability_topic"] = availability_topic.c_str();
        this->discoveryJson["state_topic"] = state_topic.c_str();
        std::string val = "{{ value_json.";
        val += _sensorClass;
        val += "}}";
        this->discoveryJson["value_template"] = val.c_str();
       /* if ((__unit.length()) > 0)
        {
            this->discoveryJson["unit_of_meas"] = __unit.c_str();
        }*/
        this->discoveryJson["payload_on"] = true;
        this->discoveryJson["payload_off"] = false;
        this->discoveryJson["device_class"] = _sensorClass.c_str();
        this->discoveryJson["name"] = unique_id.str().c_str();
        this->discovery_message = discoveryJson.dump(5);

    }
};