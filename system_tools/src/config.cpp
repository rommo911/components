#include "sdkconfig.h"

#include "config.hpp"
#include "debug_tools.h"
#ifdef ESP_PLATFORM
#include "utilities.hpp"
#endif
#include "esp_exception.hpp"
#include "esp_log.h"
#include <sstream>
#include <thread>
#include <map>
#include "FreeRTOS.hpp"
std::map<const char *, Config *, StrCompare> Config::listOfConfig{};
// std::map<std::string, std::string> Config::configHashStr{};
std::string Config::actionResponse{};
// std::string Config::CommandResponse{};

/**
 * @brief check if number is between 0 -100 %
 *
 * @param num number to be checked
 * @return esp_err_t
 */
esp_err_t Config::AssertPercentage(int num)
{
    if (num <= 100 && num >= 0)
        return ESP_OK;
    else
    {
        return ESP_ERR_INVALID_ARG;
    }
}

Config::~Config()
{
    listOfConfig.erase(m_name.c_str());
};

Config::Config(const char *name) : m_name(std::string(name))
{
    listOfConfig[m_name.c_str()] = this;
} // constructor that only copies the name // usually called with original class' TAG

/**
 * @brief
 *
 * @return true
 * @return false
 */
bool Config::Isconfigured() const
{
    return isConfigured;
};

void Config::LoadConfig()
{
    if (LoadFromNVS() != ESP_OK)
    {
        RestoreDefault();
    }
}

/**
 * @brief check if a string contain a  certain key
 *
 * @param str string to be checked
 * @param key key to search for in the string
 * @return esp_err_t
 * ESP_ERR_NOT_FOUND : not found
 * ESP_OK : key is present in the string
 */
esp_err_t Config::AssertStrFind(const std::string &str, const char *key)
{
    if (str.find(key) != -1)
        return ESP_OK;
    else
    {
        return ESP_ERR_NOT_FOUND;
    }
}

/**
 * @brief check if a string contain a  certain key
 *
 * @param str string to be checked
 * @param key key to search for in the string
 * @return esp_err_t
 * ESP_ERR_NOT_FOUND : not found
 * ESP_OK : key is present in the string
 */
esp_err_t Config::AssertStrFind(const char *str, const char *key)
{
    std::string compareString = (str);
    return AssertStrFind(str, key);
}

esp_err_t Config::AddSystemInfo(json &info)
{
#ifdef ESP_PLATFORM
    info["system"] =
        {
            {"HeapInternal", tools::dumpHeapInfo(0).c_str()},
            {"HeapTotal", tools::dumpHeapInfo(1).c_str()},
            {"ChipRevision", tools::getChipRevision()},
            {"CpuFreqMHz", tools::getCpuFreqMHz()},
            {"CycleCount", tools::getCycleCount()},
            {"SdkVersion", tools::getSdkVersion()},
            {"FlashChipSize", tools::getFlashChipSize() / 1024},
            {"FlashChipSpeed", tools::getFlashChipSpeed() / 1000000},
            {"FlashChipMode", tools::getFlashChipMode()},
            {"EfuseMac", tools::getEfuseMac()},
        };
#endif
    return ESP_OK;
}

/**
 * @brief run diagnostic on all classes attached
 *
 * @return esp_err_t
 */
esp_err_t Config::DiagnoseAll()
{
    ESP_LOGW("Diagnostic", " --> ");
    for (const auto &conf : listOfConfig)
    {
        ESP_LOGI("Diagnostic", " --> %s ", conf.first);
        try
        {
            Config *_this = conf.second;
            if (_this != nullptr)
            {
                esp_err_t ret = _this->Diagnose();
                if (ret != ESP_OK)
                {
                    ESP_LOGE("Diagnostic", " --> %s failed ", conf.first);
                }
            }
        }
        catch (const std::exception &e)
        {
            ESP_LOGE("config", "error while trying to diagnose %s : %s", conf.first, e.what());
        }
    }
    return ESP_OK;
}

esp_err_t Config::AssertjsonStr(const json &config_in, const char *key, std::string &out, uint8_t len_max, uint8_t len_min)
{
    if (config_in.contains(key))
    {
        try
        {
            std::string str;               // try to find the key
            config_in.at(key).get_to(str); // will throw if not found / will throw if not string (other type)

            if (str.length() >= len_min)
            {
                if (len_max && (str.length() > len_max))
                {
                    return ESP_ERR_INVALID_ARG;
                }
                out = str;
                this->isConfigured = true;
                return ESP_OK;
            }
        }
        catch (const std::exception &e)
        {
            ESP_LOGE("json conf parser", "error while trying to set %s : %s", key, e.what());
            return ESP_ERR_INVALID_ARG;
        }
    }
    return ESP_ERR_NOT_FOUND;
}

esp_err_t Config::AssertJsonBool(const json &config_in, const char *key, bool &out)
{
    if (config_in.contains(key))
    {
        try
        {
            bool val = false;
            config_in.at(key).get_to(val); // will throw if not found / will throw if not boolean
            out = config_in[key];
            isConfigured = true;
            return ESP_OK;
        }
        catch (const std::exception &e)
        {
            // ESP_LOGE("json conf parser", "error while trying to set %s : %s", key, e.what());
            return ESP_ERR_INVALID_ARG;
        }
    }
    return ESP_ERR_NOT_FOUND;
}

std::map<const char *, Config *, StrCompare> &Config::GetListOfConfig()
{
    return listOfConfig;
}

esp_err_t Config::_MqttCommandCallBackWrapper(const std::string &topic, const std::string &data)
{
    try
    {
        json j_parent = json::parse(data); // try to parse // will throw
        esp_err_t ret = ESP_ERR_INVALID_RESPONSE;
        if (topic.find("config") != -1)
        {
            return ParseJsonToConfig(j_parent);
        }
        if (topic.find("action") != -1)
        {
            return ParseJsonToCommands(j_parent);
        }
        return ret;
    }
    catch (const json::exception &e)
    {
        ESP_LOGE("json parser", " couldn't parse.. NOT a json or format error");
        return ESP_ERR_INVALID_STATE;
    }
    catch (const std::exception &e)
    {
        ESP_LOGE("json parser", " exception occured : %s", e.what());
        return ESP_ERR_INVALID_STATE;
    }
    return ESP_FAIL;
}

/**
 * @brief get the json string and parse it
 * if contains "command" check for what class is intended these commands (tag)
 * if match ? call the corresponding config overrided function
 *
 * @param j_parent
 * @return esp_err_t
 */
esp_err_t Config::ParseJsonToCommands(const json &j_parent)
{
    if (j_parent.contains("command")) // parsed ok ,assert object is present
    {
        if (j_parent["command"].is_object()) // Config is present, check if contains other objects inside
        {
            actionResponse = "";
            for (const auto &sub_json : j_parent["command"].items()) // get sub objects : ex rgb / wifi ..etc
            {
                auto &list = Config::GetListOfConfig();   // get configguration list
                const char *tag = sub_json.key().c_str(); // get json object key ( ex rgb )
                if (list.count(tag) > 0)                  // match the key in json object with a config tag ( exist ?)
                {
                    // matched // initiate config
                    Config *targetClass = list[tag]; // get the config class
                    if (targetClass != nullptr)
                    {
                        const json &jsonCommand = j_parent["command"]; // pass the prespective sub json object
                        esp_err_t ret = targetClass->MqttCommandCallBack(jsonCommand);
                        actionResponse += tools::stringf(" Command to %s return code %d", tag, static_cast<int>(ret));
                        if (ret == ESP_OK)
                        {
                            return ret;
                        }
                    }
                }
                else if (sub_json.key() == "system_request_reboot")
                {
                    actionResponse = (" Command to reboot OK");
                    FreeRTOS::StartTask([](void *arg)
                                        { std::this_thread::sleep_for(10s); esp_restart(); });
                    return ESP_OK;
                }
                else if (sub_json.key() == "system_request_config")
                {
                    actionResponse = Config::DumpAllJsonConfig();
                    return ESP_OK;
                }
                else if (sub_json.key() == "system_request_status")
                {
                    actionResponse = Config::DumpAllJsonStatus();
                    return ESP_OK;
                }
            }
        }
    }
    return ESP_FAIL;
}
esp_err_t Config::ParseJsonToConfig(const json &j_parent)
{
    if (j_parent.contains("config")) // parsed ok ,assert object is present
    {
        if (j_parent["config"].is_object()) // Config is present, check if contains other objects inside
        {
            actionResponse = "";
            for (const auto &sub_json : j_parent["config"].items()) // get sub objects : ex rgb / wifi ..etc
            {
                const char *tag = sub_json.key().c_str(); // get json object key ( ex rgb )
                auto &list = Config::GetListOfConfig();   // get configguration list
                if (list.count(tag) > 0)                  // match the key in json object with a config tag
                {
                    Config *targetClass = list[tag];             // get the config class
                    const json &jsonConfig = j_parent["config"]; // pass the prespective sub json object
                    if (targetClass != nullptr)
                    {
                        esp_err_t ret = targetClass->SetConfigurationParameters(jsonConfig);
                        actionResponse += tools::stringf(" config to %s returned code %d", tag, static_cast<int>(ret));
                        if (ret == ESP_OK)
                        {
                            return ret;
                        }
                        else
                        {
                            ESP_LOGE("Config", "target class %s could not handle data", tag);
                        }
                    }
                }
                else
                {
                    ESP_LOGE("Config", "target class '%s' not found", tag);
                }
            }
        }
    }
    return ESP_FAIL;
}

std::string Config::DumpAllJsonConfig()
{
    const auto &listOfAllConfigClasses = Config::GetListOfConfig();
    std::string retStr;
    json FullConfig = {};
    FullConfig["config"] = {};
    for (const auto &conf : listOfAllConfigClasses)
    {
        Config *_this_config = conf.second;
        if (_this_config != nullptr)
        {
            _this_config->GetConfiguration(FullConfig["config"]);
        }
    }
    retStr = FullConfig.dump(-1, ' ', true);
    // ESP_LOGI("config dump", "%s", retStr.c_str());
    return retStr;
}

std::string Config::DumpAllJsonStatus()
{
    const auto &list = Config::GetListOfConfig();
    std::string retStr;
    json FullStatus = {};
    FullStatus["status"] = {};
    for (const auto &[tag, configPointer] : list)
    {
        Config *_this = configPointer;
        if (_this != nullptr)
        {
            _this->GetConfigurationStatus(FullStatus["status"]);
        }
    }
    retStr = FullStatus.dump(-1, ' ', true);
    ESP_LOGI("status", "%s", retStr.c_str());
    return retStr;
}
/*
const std::stringstream &Config::GetConfigHashString()
{
    static std::stringstream hashStream;
    hashStream.str("");
    for (const auto &conf : configHashStr)
    {
        hashStream << conf.first << "=" << conf.second << ".";
    }
    return hashStream;
}
*/
/*
esp_err_t Config::AddConfigToHash(const std::string &key, const char *val) noexcept(false)
{
    try
    {
        configHashStr[key] = (val);
        return ESP_OK;
    }
    catch (const std::exception &e)
    {
        ESP_LOGE("conf hash", "error while trying to pasre %s : %s", key.c_str(), e.what());
    }
    return ESP_FAIL;
}
*/
esp_err_t Config::Diagnose()
{
    ESP_LOGW(m_name.c_str(), "NO  Diagnose overrivde");
    return ESP_OK;
};

esp_err_t Config::RestoreDefault()
{
    ESP_LOGE(m_name.c_str(), "NO RestoreDefault config overrivde");
    return ESP_OK;
};
// load params from NVS
esp_err_t Config::LoadFromNVS()
{
    ESP_LOGE(m_name.c_str(), "NO LoadFromNVS overrivde");
    return ESP_OK;
}
// save params to NVS
esp_err_t Config::SaveToNVS()
{
    ESP_LOGE(m_name.c_str(), "NO SaveToNVS overrivde");
    return ESP_OK;
};
// exctract configuration from a json object and set them , not all params are required
esp_err_t Config::SetConfigurationParameters(const json &config_in)
{
    ESP_LOGE(m_name.c_str(), "NO SetConfigurationParameters overrivde");
    return ESP_OK;
};
// return json with all current configured parameters
esp_err_t Config::GetConfiguration(json &config_out) const
{
    ESP_LOGE(m_name.c_str(), "NO GetConfiguration overrivde");
    return ESP_OK;
};
// return json with all current status values
esp_err_t Config::GetConfigurationStatus(json &config_out) const
{
    ESP_LOGE(m_name.c_str(), "NO GetConfigurationStatus overrivde");
    return ESP_OK;
};

esp_err_t Config::MqttCommandCallBack(const json &config_in)
{
    ESP_LOGE(m_name.c_str(), "NO MqttCommandCallBack overrivde");
    return ESP_FAIL;
}