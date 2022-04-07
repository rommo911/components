#ifndef ___CONFIG_H__
#define ___CONFIG_H__
#pragma once

#include "utilities.hpp"
#include "sdkconfig.h"
#include <cstring>
#include <functional>
#include <list>
#include <map>
#include <nlohmann/json.hpp>
#include <stdint.h>
#include <string>
#include <vector>
using json = nlohmann::json; // json
typedef std::function<esp_err_t(const std::string& topic, const std::string& data, void* arg)> mqtt_data_callback_t;

struct mqtt_data_callback_describtor_t // used to store prameters of callback// copy from // copied to prevent including mqtt here
{
    mqtt_data_callback_t callback;
    void* argument;
    const std::string info;
};

class Config // base class // need to be used with another class abstract explicitely
{
    private:
    static std::string actionResponse;
    static std::map<const char*, Config*, StrCompare> listOfConfig; // list where all classes pointer are stored when calling  InitThisConfig()
    static std::map<std::string, std::string> configHashStr;
    static const int STR_MAX_SIZE = 15;
    static std::list<std::string> errorCodes;   // some error coeds to be stored // not used
    static std::list<std::string> warningCodes; // some error coeds to be stored // not used
    std::string m_name;                         // member name // used to differ from the abstract class' TAG, connot copy the original class' TAG
    public:
    // contructor : initialize variable name/isconfigured ..etc
    explicit Config(const char* name) : m_name(std::string(name)) {} // constructor that only copies the name // usually called with original class' TAG
    //
    virtual ~Config() {};
    // return list of classes
    static std::map<const char*, Config*, StrCompare>& GetListOfConfig();
    // returns true if setting were configured (by DOL), false if factory default config is active
    bool Isconfigured() const;
    // restore factory default static params and save them to NVS, this will reset isConfigured flasg
    //* static functions
    //
    // add system vitals info to a json object //
    // launch some diagnostic routine and return value : u have to define this function manually and adapt it to your
    //  pure virtual // to force class to be abstract
    virtual esp_err_t Diagnose() { return ESP_OK; };
    //
    const std::string& GetConfigName() { return m_name; }
    // launch diagnostic on the list of config classes
    // take json object , search for a match a,d call setConfiguration
    // DEBUG

    std::string GetConfigParamStr(const char* key)const;

    template <typename T>
    T GetConfigParam(const char* key) const
    {
        json conf;
        this->GetConfiguration(conf);
        conf = conf;
        T ret{};
        conf[this->m_name.c_str()]["params"].at(key).get_to(ret);
        return ret;
    }

    private:
    friend class ConfigAction;
    static esp_err_t _MqttCommandCallBackWrapper(const std::string& topic, const std::string& data, void* arg);

    template <typename T>
    esp_err_t AddConfigToHash(const std::string& key, const T& val) noexcept(false);
    //
    static esp_err_t AddConfigToHash(const std::string& key, const char* val) noexcept(false);
    //
    static const std::stringstream& GetConfigHashString();
    // return all registered errors during config/diagnostic
    static std::list<std::string> GetErrorCodes();
    // return all registered warning during config/diagnostics
    static std::list<std::string> GetWarningCodes();
    static std::string DumpAllJsonConfig();
    static std::string DumpAllJsonStatus();
    static esp_err_t ParseJsonToConfig_Commands(const std::string& json_str) noexcept(false);
    static esp_err_t DiagnoseAll();
    static const std::string& GetActionResponse() { return actionResponse; }
    static bool ErrosOccured();
    static bool WarningsOccured();
    static esp_err_t AddSystemInfo(json&);

    protected:
    bool isConfigured = false;
    virtual esp_err_t RestoreDefault() { return ESP_OK; };
    // load params from NVS
    virtual esp_err_t LoadFromNVS() { return ESP_OK; }
    // save params to NVS
    virtual esp_err_t SaveToNVS() { return ESP_OK; };
    // exctract configuration from a json object and set them , not all params are required
    virtual esp_err_t SetConfigurationParameters(const json& config_in) { return ESP_OK; };
    // return json with all current configured parameters
    virtual esp_err_t GetConfiguration(json& config_out) const { return ESP_OK; };
    // return json with all current status values
    virtual esp_err_t GetConfigurationStatus(json& config_out) const { return ESP_OK; };

    virtual esp_err_t MqttCommandCallBack(const json& config_in);
    // the static wrapper to all mqtt callbacks
    //  add "this" to the Config* List (listOfConfig) // + load from NVS
    void InitThisConfig();
    // remove "this" from the Config* List (listOfConfig)
    void DettachThisConfig();
    // add a string that describes a warning occurence
    void AddErrorCode(const std::string&);
    // add a string that describes an error occurence
    void AddWarningCode(const std::string&);
    //* static functions
    // check if num is between 0-100
    static esp_err_t AssertPercentage(int num) noexcept(false);
    // check if number is between min & max
    template <typename T, typename T2>
    static esp_err_t AssertNumber(T& num, const T2 min, const T2 max) noexcept;
    // check if string contains & key ( char or string)
    static esp_err_t AssertStrFind(const std::string&, const char*) noexcept(false);
    // check if string (char*) contains & key ( char or string)
    static esp_err_t AssertStrFind(const char*, const char*) noexcept(false);
    //
    template <typename r, typename p, typename r_min, typename p_min, typename r_max, typename p_max>
    static esp_err_t AssertDuration(const std::chrono::duration<r, p>& num, const std::chrono::duration<r_min, p_min>& _min, const std::chrono::duration<r_max, p_max>& _max) noexcept;
    //
    esp_err_t AssertjsonStr(const json& config_in, const char* key, std::string& out, uint8_t len_max = 0, uint8_t len_min = 0) noexcept(false);
    //
    esp_err_t AssertjsonBool(const json& config_in, const char* key, bool& out) noexcept(false);
    //
    template <typename T, typename T2>
    esp_err_t AssertjsonInt(const json& config_in, const char* key, T& out, const T2 _min, const T2 _max) noexcept(false);
    template <typename r, typename p, typename r_min, typename p_min, typename r_max, typename p_max>
    esp_err_t AssertjsonDuration(const json& config_in, const char* key, std::chrono::duration<r, p>& out, const std::chrono::duration<r_min, p_min>& _min, const std::chrono::duration<r_max, p_max>& _max);
};

template <typename T, typename T2>
esp_err_t Config::AssertNumber(T& num, const T2 min, const T2 max) noexcept
{
    if (num <= max && num >= min)
    {
        return ESP_OK;
    }
    else
    {
        return ESP_ERR_INVALID_ARG;
    }
}

template <typename r, typename p, typename r_min, typename p_min, typename r_max, typename p_max>
esp_err_t Config::AssertDuration(const std::chrono::duration<r, p>& num, const std::chrono::duration<r_min, p_min>& _min, const std::chrono::duration<r_max, p_max>& _max) noexcept
{
    if (num <= _max && num >= _min)
        return ESP_OK;
    else
    {
        return ESP_ERR_INVALID_ARG;
    }
}

template <typename T, typename T2>
esp_err_t Config::AssertjsonInt(const json& config_in, const char* key, T& out, const T2 _min, const T2 _max) noexcept(false)
{
    try
    {
        if (config_in.contains(key))
        {
            const T& _val = config_in.at(key);
            if (AssertNumber(_val, _min, _max) == ESP_OK)
            {
                out = _val;
                isConfigured = true;
                return ESP_OK;
            }
            else
            {
                return ESP_ERR_INVALID_ARG;
            }
        }
    }
    catch (const std::exception& e)
    {
        ESP_LOGE("json conf parser", "error while trying to pasre %s : %s", key, e.what());
    }
    return ESP_ERR_NOT_FOUND;
}

template <typename r, typename p, typename r_min, typename p_min, typename r_max, typename p_max>
esp_err_t Config::AssertjsonDuration(const json& config_in, const char* key, std::chrono::duration<r, p>& out, const std::chrono::duration<r_min, p_min>& _min, const std::chrono::duration<r_max, p_max>& _max) noexcept(false)
{
    try
    {
        if (config_in.contains(key))
        {
            const auto _val = std::chrono::milliseconds(config_in.at(key));
            if (AssertDuration(_val, _min, _max) == ESP_OK)
            {
                out = std::chrono::duration_cast<std::chrono::duration<r, p>>(_val);
                isConfigured = true;
                return ESP_OK;
            }
            else
            {
                return ESP_ERR_INVALID_ARG;
            }
        }
    }
    catch (const std::exception& e)
    {
        ESP_LOGE("json conf parser", "error while trying to pasre %s : %s", key, e.what());
    }
    return ESP_ERR_NOT_FOUND;
}

template <typename T>
esp_err_t Config::AddConfigToHash(const std::string& key, const T& val) noexcept(false)
{
    try
    {
        configHashStr[key] = std::to_string(val);
        return ESP_OK;
    }
    catch (const std::exception& e)
    {
        ESP_LOGE("conf hash", "error while trying to pasre %s : %s", key.c_str(), e.what());
    }

    return ESP_FAIL;
}

// helper class to access Config private functions
class ConfigAction : private Config
{

    public:
    ConfigAction() : Config("General Config Helper ") {};
    ~ConfigAction();
    static std::string PrintDumpAllJsonConfig() { return DumpAllJsonConfig(); };
    static std::string PrintDumpAllJsonStatus() { return DumpAllJsonStatus(); };
    static esp_err_t RunDiagnoseAll() { return DiagnoseAll(); };
    static const std::string& LastActionResponse() { return GetActionResponse(); }
    static bool IsErrosOccured() { return ErrosOccured(); };
    static bool IsWarningsOccured() { return WarningsOccured(); };
    static esp_err_t AddSystemInfotoJson(json& j) { return AddSystemInfo(j); };
    static esp_err_t MqttCommandCallBackWrapper(const std::string& topic, const std::string& data, void* arg) // helper to access private function from Config class
    {
        return _MqttCommandCallBackWrapper(topic, data, arg);
    }
};

template <typename r, typename p>
static inline unsigned int ToMs(const std::chrono::duration<r, p>& in)
{
    return (unsigned int)std::chrono::duration_cast<std::chrono::milliseconds>(in).count();
}

#endif // __CONFIG_NVS_H__