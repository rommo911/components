#include <HTTPClient.h>
#include "nvs_tools.h"
#include <nlohmann/json.hpp>
#include "openWeather.hpp"
#include "esp_log.h"

#define NVS_OW_NAME_SAPCE "OPEN_WEATHER"// or °F
#define NVS_KEY_OW_UNIT "ow_unit"// or °F
#define NVS_KEY_OW_API_KEY  "ow_api"
#define NVS_KEY_OW_CITY     "ow_city"  
#define NVS_KEY_OW_COUNTRY_CODE "ow_cc"

OpenWeather::OpenWeather(const char* _city, const char* _countryCode, const char* _unit, const char* _apiKey)
{
    this->city = _city;
    this->countryCode = _countryCode;
    this->unit = _unit;
    this->apiKey = _apiKey;
    IsInitialized = true;
    serverPath = "http://api.openweathermap.org/data/2.5/weather?q=";
    serverPath += this->city.c_str();
    serverPath += ",";
    serverPath += this->countryCode.c_str();
    serverPath += "&APPID=";
    serverPath += this->apiKey.c_str();
    serverPath += "&units=";
    serverPath += this->unit.c_str();

}

OpenWeather::OpenWeather()
{
    if (ReadConfig() == ESP_OK)
    {
        IsInitialized = true;
        serverPath = "http://api.openweathermap.org/data/2.5/weather?q=";
        serverPath += city.c_str();
        serverPath += ",";
        serverPath += countryCode.c_str();
        serverPath += "&APPID=";
        serverPath += apiKey.c_str();
        serverPath += "&units=";
        serverPath += unit.c_str();
    }
    else
    {
        IsInitialized = false;
    }
}

using namespace nlohmann;

esp_err_t OpenWeather::ReadConfig()
{
    esp_err_t ret = ESP_FAIL;
    NvsPointer nvs = OPEN_NVS_DEAFULT();
    if (nvs->isOpen())
    {
        ret = nvs->getS(NVS_KEY_OW_API_KEY, this->apiKey);
        ret |= nvs->getS(NVS_KEY_OW_CITY, this->city);
        ret |= nvs->getS(NVS_KEY_OW_COUNTRY_CODE, this->countryCode);
        ret |= nvs->getS(NVS_KEY_OW_UNIT, this->unit);
    }
    else
    {
        ESP_LOGE(NVS_OW_NAME_SAPCE, "NVS_KEY_OW_API_KEY not found");
        IsInitialized = false;
        return ESP_ERR_NOT_FOUND;
    }
    IsInitialized = ret == ESP_OK ? true : false;
    return ret;
}

#define GET_NOW_MINUTES (std::chrono::duration_cast<std::chrono::minutes>(std::chrono::high_resolution_clock::now().time_since_epoch()).count() )


bool OpenWeather::getWeather()
{
    if (!IsInitialized)
    {
        ESP_LOGE(NVS_OW_NAME_SAPCE, "NOT INITIALIZED");
        return false;
    }
    if (updated && ((GET_NOW_MINUTES - this->lastUpdated) < 10))
    {
        return true;
    }
    result = httpGETRequest(serverPath.c_str()).c_str();
    if (result.length() > 150)
    {
        this->jsonData.clear();
        try
        {
            jsonData = json::parse(result);
            const auto& json_weather = jsonData.at("weather")[0];
            json_weather.at("main").get_to<std::string>(this->weatherInfo.description);
            json_weather.at("icon").get_to<std::string>(this->weatherInfo.icon);
            const auto& json_main = jsonData.at("main");
            json_main.at("feels_like").get_to<float>(weatherInfo.feels_like);
            json_main.at("temp_max").get_to<float>(weatherInfo.temp_max);
            json_main.at("temp_min").get_to<float>(weatherInfo.temp_min);
            json_main.at("humidity").get_to<float>(weatherInfo.humidity);
            json_main.at("pressure").get_to<int>(weatherInfo.pressure);
            json_main.at("temp").get_to<float>(weatherInfo.temp);
            jsonData.at("wind").at("speed").get_to<float>(weatherInfo.wind_speed);
            updated = true;
            lastUpdated = GET_NOW_MINUTES;
            return true;
        }
        catch (const std::exception& e)
        {
            updated = false;
            ESP_LOGE("WEATHER", "%s", e.what());
        }
    }
    updated = false;
    return false;

}
String OpenWeather::httpGETRequest(const char* serverName) {
    HTTPClient http;
    http.begin(serverName);
    int httpResponseCode = http.GET();
    String payload = "";
    if (httpResponseCode > 0) {

        payload = http.getString();
    }
    else {
        Serial.print("Error code: ");
        Serial.println(httpResponseCode);
    }
    http.end();
    return payload;
}


const std::string& OpenWeather::getCity()const
{
    return city;
}
const std::string& OpenWeather::getWeatherDescription() const
{
    return this->weatherInfo.description;

}
const std::string& OpenWeather::getWeatherIcon() const
{
    return this->weatherInfo.icon;
}

const float& OpenWeather::getWeatherFeelsLike()const
{

    return this->weatherInfo.feels_like;
}

const  float& OpenWeather::getWeatherHumidity()const
{
    return this->weatherInfo.humidity;
}
const float& OpenWeather::getWeatherTemperatureMax()const
{
    return this->weatherInfo.temp_max;
}
const  int& OpenWeather::getWeatherPressure()const
{
    return this->weatherInfo.pressure;
}
const float& OpenWeather::getWeatherTemperatureMin()const
{
    return this->weatherInfo.temp_min;
}

const float& OpenWeather::getWeatherWindSpeed()const
{
    return this->weatherInfo.wind_speed;
}
