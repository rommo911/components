#ifndef __ALADIN_CONFIG_H__
#define __ALADIN_CONFIG_H__
#pragma once

#include "sdkconfig.h"
#include <cstring>
#include <functional>
#include <list>
#include <map>
#include <nlohmann/json.hpp>
#include <stdint.h>
#include <string>
#include <vector>
#include <chrono>
#include "utilities.hpp"
using json = nlohmann::json; // json
using namespace std::chrono;

typedef std::function<esp_err_t(const char *topicBuffer, const size_t topic_len, const char *dataBuffer, const size_t data_len, void *arg)> mqtt_data_callback_t;

class Config // base class // need to be used with another class abstract explicitely
{
private:
    static std::string actionResponse;
    static std::map<const char *, Config *, StrCompare> listOfConfig; // list where all classes pointer are stored when calling  InitThisConfig()
    static const int STR_MAX_SIZE = 15;
    const std::string m_name; // member name // used to differ from the abstract class' TAG, connot copy the original class' TAG
public:
    // contructor : initialize variable name/isconfigured ..etc
    Config(const char *name);
    Config() = delete;
    Config(Config &) = delete;
    Config(Config &&) = delete;
    //
    ~Config();
    void LoadConfig();

    // return list of classes
    static std::map<const char *, Config *, StrCompare> &GetListOfConfig();
    // returns true if setting were configured (by DOL), false if factory default config is active
    bool Isconfigured() const;
    // restore factory default static params and save them to NVS, this will reset isConfigured flasg
    // launch some diagnostic routine and return value : u have to define this function manually and adapt it to your
    // to force class to be abstract
    virtual esp_err_t Diagnose();
    //
    const std::string &GetConfigName() { return m_name; }
    // launch diagnostic on the list of config classes
    // take json object , search for a match a,d call setConfiguration
    // DEBUG

private:
    friend class ConfigAction;
    static esp_err_t _MqttCommandCallBackWrapper(const std::string &topic, const std::string &data);

    template <typename T>
    esp_err_t AddConfigToHash(const std::string &key, const T &val) noexcept(false);
    //
    static esp_err_t AddConfigToHash(const std::string &key, const char *val) noexcept(false);
    //
    // static const std::stringstream &GetConfigHashString();
    // NOT USED
    static std::string DumpAllJsonConfig();
    static std::string DumpAllJsonStatus();
    static esp_err_t ParseJsonToConfig(const json &j_parent) noexcept(false);
    static esp_err_t ParseJsonToCommands(const json &j_parent) noexcept(false);

    static esp_err_t DiagnoseAll();
    static const std::string &GetActionResponse() { return actionResponse; }
    static esp_err_t AddSystemInfo(json &);

protected:
    bool isConfigured = false;
    virtual esp_err_t RestoreDefault();
    // load params from NVS
    virtual esp_err_t LoadFromNVS();
    // save params to NVS
    virtual esp_err_t SaveToNVS();
    // exctract configuration from a json object and set them , not all params are required
    virtual esp_err_t SetConfigurationParameters(const json &config_in);
    // return json with all current configured parameters
    virtual esp_err_t GetConfiguration(json &config_out) const;
    // return json with all current status values
    virtual esp_err_t GetConfigurationStatus(json &config_out) const;

    virtual esp_err_t MqttCommandCallBack(const json &config_in);
    // the static wrapper to all mqtt callbacks
    /**
     * @brief
     *
     */
    // add a string that describes a warning occurence
    //* static functions
    // check if num is between 0-100
    static esp_err_t AssertPercentage(int num) noexcept(false);
    // check if number is between min & max
    template <typename T, typename T2>
    static esp_err_t AssertNumber(T &num, const T2 min, const T2 max) noexcept;
    // check if string contains & key ( char or string)
    static esp_err_t AssertStrFind(const std::string &, const char *) noexcept(false);
    // check if string (char*) contains & key ( char or string)
    static esp_err_t AssertStrFind(const char *, const char *) noexcept(false);
    //
    template <typename r, typename p, typename r_min, typename p_min, typename r_max, typename p_max>
    static esp_err_t AssertDuration(const duration<r, p> &num, const duration<r_min, p_min> &_min, const duration<r_max, p_max> &_max) noexcept;
    //
    esp_err_t AssertjsonStr(const json &config_in, const char *key, std::string &out, uint8_t len_max = 0, uint8_t len_min = 0) noexcept(false);
    //
    esp_err_t AssertJsonBool(const json &config_in, const char *key, bool &out) noexcept(false);
    //
    template <typename T, typename T2>
    esp_err_t AssertJsonInt(const json &config_in, const char *key, T &out, const T2 _min, const T2 _max) noexcept(false);
    template <typename r, typename p, typename r_min, typename p_min, typename r_max, typename p_max>
    esp_err_t AssertJsonDuration(const json &config_in, const char *key, duration<r, p> &out, const duration<r_min, p_min> &_min, const duration<r_max, p_max> &_max);
    //
    template <typename T>
    esp_err_t AssertjsonEnum(const json &config_in, const char *key, T &out, const T _min, const T _max) noexcept(false);
};

template <typename T, typename T2>
esp_err_t Config::AssertNumber(T &num, const T2 min, const T2 max) noexcept
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
esp_err_t Config::AssertDuration(const duration<r, p> &num, const duration<r_min, p_min> &_min, const duration<r_max, p_max> &_max) noexcept
{
    if (num <= _max && num >= _min)
        return ESP_OK;
    else
    {
        return ESP_ERR_INVALID_ARG;
    }
}

template <typename T>
esp_err_t Config::AssertjsonEnum(const json &config_in, const char *key, T &out, const T _min, const T _max) noexcept(false)
{
    try
    {
        if (config_in.contains(key))
        {
            uint8_t _val = config_in.at(key);
            if (AssertNumber(_val, static_cast<uint8_t>(_min), static_cast<uint8_t>(_max)) == ESP_OK)
            {
                out = static_cast<T>(_val);
                isConfigured = true;
                return ESP_OK;
            }
            else
            {
                return ESP_ERR_INVALID_ARG;
            }
        }
    }
    catch (const std::exception &e)
    {
        ESP_LOGE("json conf parser", "error while trying to pasre %s : %s", key, e.what());
    }
    return ESP_ERR_NOT_FOUND;
}

template <typename T, typename T2>
esp_err_t Config::AssertJsonInt(const json &config_in, const char *key, T &out, const T2 _min, const T2 _max) noexcept(false)
{
    try
    {
        if (config_in.contains(key))
        {
            T _val = config_in.at(key);
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
    catch (const std::exception &e)
    {
        ESP_LOGE("json conf parser", "error while trying to pasre %s : %s", key, e.what());
    }
    return ESP_ERR_NOT_FOUND;
}

template <typename r, typename p, typename r_min, typename p_min, typename r_max, typename p_max>
esp_err_t Config::AssertJsonDuration(const json &config_in, const char *key, duration<r, p> &out, const duration<r_min, p_min> &_min, const duration<r_max, p_max> &_max) noexcept(false)
{
    try
    {
        if (config_in.contains(key))
        {
            const auto _val = milliseconds(config_in.at(key));
            if (AssertDuration(_val, _min, _max) == ESP_OK)
            {
                out = duration_cast<duration<r, p>>(_val);
                isConfigured = true;
                return ESP_OK;
            }
            else
            {
                return ESP_ERR_INVALID_ARG;
            }
        }
    }
    catch (const std::exception &e)
    {
        ESP_LOGE("json conf parser", "error while trying to pasre %s : %s", key, e.what());
    }
    return ESP_ERR_NOT_FOUND;
}

/*template <typename T>
esp_err_t Config::AddConfigToHash(const std::string &key, const T &val) noexcept(false)
{
    try
    {
        configHashStr[key] = std::to_string(val);
        return ESP_OK;
    }
    catch (const std::exception &e)
    {
        ESP_LOGE("conf hash", "error while trying to pasre %s : %s", key.c_str(), e.what());
    }

    return ESP_FAIL;
}*/

// helper class to access Config private functions
class ConfigAction
{
public:
    ConfigAction();
    ~ConfigAction();
    static std::string PrintDumpAllJsonConfig() { return Config::DumpAllJsonConfig(); };
    static std::string PrintDumpAllJsonStatus() { return Config::DumpAllJsonStatus(); };
    static esp_err_t RunDiagnoseAll() { return Config::DiagnoseAll(); };
    static const std::string &LastActionResponse() { return Config::GetActionResponse(); }
    static esp_err_t AddSystemInfotoJson(json &j) { return Config::AddSystemInfo(j); };
    static esp_err_t MqttCommandCallBackWrapper(const std::string &topic, const std::string &data) // helper to access private function from Config class
    {
        return Config::_MqttCommandCallBackWrapper(topic, data);
    }
};

template <typename r, typename p>
static inline unsigned int ToMs(const duration<r, p> &duration)
{
    return (unsigned int)duration_cast<milliseconds>(duration).count();
}

#endif // __ALADIN_CONFIG_NVS_H__