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

std::list<std::string> Config::errorCodes{};
std::list<std::string> Config::warningCodes{};
std::map<const char*, Config*, StrCompare> Config::listOfConfig{};
std::map<std::string, std::string> Config::configHashStr{};
std::string Config::actionResponse{};
/**
 * @brief add an error when ocuured
 *
 * @param str
 */
void Config::AddErrorCode(const std::string& str)
{
    errorCodes.push_back(m_name + " " + str);
}

/**
 * @brief add a warning strinf
 *
 * @param str
 */
void Config::AddWarningCode(const std::string& str)
{
    warningCodes.push_back(m_name + " " + str);
}

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

/**
 * @brief add this inherited class to the static lsit
 *
 */
void Config::InitThisConfig()
{
    listOfConfig[m_name.c_str()] = this;
    if (LoadFromNVS() != ESP_OK)
    {
        RestoreDefault();
    }
}

/**
 * @brief should be called at deinit of prespective class
 *
 */
void Config::DettachThisConfig()
{
    listOfConfig.erase(m_name.c_str());
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
esp_err_t Config::AssertStrFind(const std::string& str, const char* key)
{
    if (str.find(key) != -1)
        return ESP_OK;
    else
    {
        throw(ESPException("string doesn't contain what needed", "/Config assert strfind"));
    }
    return ESP_ERR_NOT_FOUND;
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
esp_err_t Config::AssertStrFind(const char* str, const char* key)
{
    std::string compareString = (str);
    return AssertStrFind(str, key);
}

/**
 * @brief get all error codes
 *
 * @return std::vector<std::string>
 */
std::list<std::string> Config::GetErrorCodes()
{
    return errorCodes;
}

/**
 * @brief get alll warning codes
 *
 * @return std::vector<std::string>
 */
std::list<std::string> Config::GetWarningCodes()
{
    return warningCodes;
}

/**
 * @brief check if any errors were added
 *
 * @return true
 * @return false
 */
bool Config::ErrosOccured()
{
    return (errorCodes.size() > 0);
}

/**
 * @brief check if any warnings were added
 *
 * @return true
 * @return false
 */
bool Config::WarningsOccured()
{
    return (warningCodes.size() > 0);
}

esp_err_t Config::AddSystemInfo(json& info)
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
        {"FlashChipSize", tools::getFlashChipSize()},
        {"FlashChipSpeed", tools::getFlashChipSpeed()},
        {"FlashChipMode", tools::getFlashChipMode()},
        {"SketchSize", tools::getSketchSize()},
        {"SketchMD5", tools::getSketchMD5().c_str()},
        {"OTASketchMD5", tools::getOTASketchMD5()},
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
    for (const auto& conf : listOfConfig)
    {
        ESP_LOGI("Diagnostic", " --> %s ", conf.first);
        try
        {
            Config* _this = conf.second;
            if (_this != nullptr)
            {
                esp_err_t ret = _this->Diagnose();
                if (ret != ESP_OK)
                {
                    ESP_LOGE("Diagnostic", " --> %s failed ", conf.first);
                }
            }
        }
        catch (const std::exception& e)
        {
            ESP_LOGE("config", "error while trying to diagnose %s : %s", conf.first, e.what());
        }
    }
    return ESP_OK;
}

esp_err_t Config::AssertjsonStr(const json& config_in, const char* key, std::string& out, uint8_t len_max, uint8_t len_min)
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
                    ESP_LOGE("json conf parser", "error while trying to set %s", str.c_str());
                }
                out = str;
                this->isConfigured = true;
                return ESP_OK;
            }
        }
        catch (const std::exception& e)
        {
            ESP_LOGE("json conf parser", "error while trying to set %s : %s", key, e.what());
            return ESP_ERR_INVALID_ARG;
        }
    }
    return ESP_ERR_NOT_FOUND;
}

esp_err_t Config::AssertjsonBool(const json& config_in, const char* key, bool& out)
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
        catch (const std::exception& e)
        {
            // ESP_LOGE("json conf parser", "error while trying to set %s : %s", key, e.what());
            return ESP_ERR_INVALID_ARG;
        }
    }
    return ESP_ERR_NOT_FOUND;
}

std::map<const char*, Config*, StrCompare>& Config::GetListOfConfig()
{
    return listOfConfig;
}

esp_err_t Config::_MqttCommandCallBackWrapper(const std::string& topic, const std::string& data, void* arg)
{
    return ParseJsonToConfig_Commands(data);
}

esp_err_t Config::ParseJsonToConfig_Commands(const std::string& json_str)
{
    try // try tp parse in first place !
    {
        json j_parent = json::parse(json_str); // try to parse // will throw
        if (j_parent.contains("config"))       // parsed ok ,assert object is present
        {
            if (j_parent["config"].is_object()) // Config is present, check if contains other objects inside
            {
                actionResponse = "";
                for (const auto& sub_json : j_parent["config"].items()) // get sub objects : ex rgb / wifi ..etc
                {
                    const char* tag = sub_json.key().c_str(); // get json object key ( ex rgb )
                    auto& list = Config::GetListOfConfig();   // get configguration list
                    if (list.count(tag) > 0)                  // match the key in json object with a config tag
                    {
                        // matched // initiate config
                        // std::string str_ = j_parent["config"].dump(2);
                        // ESP_LOGI("json", "%s", str_.c_str());
                        Config* targetClass = list[tag];             // get the config class
                        const auto& jsonConfig = j_parent["config"]; // pass the prespective sub json object
                        if (targetClass != nullptr)
                        {
                            esp_err_t ret = targetClass->SetConfigurationParameters(jsonConfig);
                            actionResponse += tools::stringf(" config to %s returned code %d", tag, static_cast<int>(ret));
                            if (ret == ESP_OK)
                            {
                                return ret;
                            }
                        }
                    }
                }
            }
        }
        else
        {
            if (j_parent.contains("command")) // parsed ok ,assert object is present
            {
                if (j_parent["command"].is_object()) // Config is present, check if contains other objects inside
                {
                    actionResponse = "";
                    for (const auto& sub_json : j_parent["command"].items()) // get sub objects : ex rgb / wifi ..etc
                    {
                        auto list = Config::GetListOfConfig();    // get configguration list
                        const char* tag = sub_json.key().c_str(); // get json object key ( ex rgb )
                        if (list.count(tag) > 0)                  // match the key in json object with a config tag
                        {
                            // matched // initiate config
                            Config* targetClass = list[tag];               // get the config class
                            const auto& jsonCommand = j_parent["command"]; // pass the prespective sub json object
                            if (targetClass != nullptr)
                            {
                                esp_err_t ret = targetClass->MqttCommandCallBack(jsonCommand);
                                actionResponse += tools::stringf(" Command to %s return with code %d", tag, static_cast<int>(ret));
                                if (ret == ESP_OK)
                                {
                                    return ret;
                                }
                            }
                        }
                        else if (sub_json.key() == "system_request_reboot")
                        {
                            actionResponse = (" Command to reboot OK");
                            //esp_restart();
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
        }
    }
    catch (const json::exception& e)
    {
        ESP_LOGE("json parser", " couldn't parse..  error occured : %s", e.what());
        return ESP_FAIL;
    }
    catch (const std::exception& e)
    {
        ESP_LOGE("json parser", "  error occured : %s", e.what());
        return ESP_FAIL;
    }
    return ESP_FAIL;
}

std::string Config::DumpAllJsonConfig()
{
    auto listOfAllConfigClasses = Config::GetListOfConfig();
    std::string retStr;
    json FullConfig = {};
    FullConfig["config"] = {};
    for (const auto& conf : listOfAllConfigClasses)
    {
        Config* _this_config = conf.second;
        if (_this_config != nullptr)
        {
            _this_config->GetConfiguration(FullConfig["config"]);
        }
    }
    try
    {
        retStr = FullConfig.dump(5);
        ESP_LOGI("config", "%s", retStr.c_str());
    }
    catch (const std::exception& e)
    {
        ESP_LOGE("config exception ", " --> %s ", e.what());
    }
    return FullConfig.dump();
}

std::string Config::DumpAllJsonStatus()
{
    auto list = Config::GetListOfConfig();
    std::string retStr;
    json FullStatus = {};
    FullStatus["status"] = {};
    for (const auto& conf : list)
    {
        Config* _this = conf.second;
        if (_this != nullptr)
        {
            _this->GetConfigurationStatus(FullStatus["status"]);
        }
    }
    try
    {
        retStr = FullStatus.dump(5);
        ESP_LOGI("status", "%s", retStr.c_str());
    }
    catch (const std::exception& e)
    {
        ESP_LOGE("status exception ", " --> %s ", e.what());
    }

    return FullStatus.dump();
}

esp_err_t Config::MqttCommandCallBack(const json& commandIn)
{
    return ESP_FAIL;
}


std::string Config::GetConfigParamStr(const char* key)const
{
    json conf = {};
    this->GetConfiguration(conf);
    conf = conf;
    std::string ret{ "" };
    conf[this->m_name.c_str()]["params"].at(key).get_to(ret);
    return ret;
}
const std::stringstream& Config::GetConfigHashString()
{
    static std::stringstream hashStream;
    hashStream.str("");
    for (const auto& conf : configHashStr)
    {
        hashStream << conf.first << "=" << conf.second << ".";
    }
    return hashStream;
}

esp_err_t Config::AddConfigToHash(const std::string& key, const char* val) noexcept(false)
{
    try
    {
        configHashStr[key] = (val);
        return ESP_OK;
    }
    catch (const std::exception& e)
    {
        ESP_LOGE("conf hash", "error while trying to pasre %s : %s", key.c_str(), e.what());
    }
    return ESP_FAIL;
}
