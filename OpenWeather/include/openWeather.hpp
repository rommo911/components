
#include <nlohmann/json.hpp>


class OpenWeather
{
    public:
    OpenWeather(const char* city, const char* countryCode, const char* unit, const char* apiKey);
    OpenWeather();
    ~OpenWeather();
    esp_err_t ReadConfig();
    bool getWeather();
    const std::string& getCity()const;
    const  std::string& getWeatherDescription()const;
    const  std::string& getWeatherIcon()const;
    const   float& getWeatherFeelsLike()const;
    const  float& getWeatherHumidity()const;
    const  float& getWeatherTemperatureMax()const;
    const  int& getWeatherPressure()const;
    const   float& getWeatherTemperatureMin()const;
    const  float& getWeatherWindSpeed()const;
    esp_err_t SetConfig(const char* _apikey, const char* _city, const char* _countryCode, const char* _unit);


    private:
    bool updated = false;
    std::string serverPath;
    std::string result;
    int64_t lastUpdated = 0;
    nlohmann::json jsonData;
    std::string  city = "poitiers";
    std::string  countryCode = "FR";
    std::string  unit = "metric";
    std::string  apiKey = "97cec37fde60c24ccb3b9be70eeab757";
    bool IsInitialized = false;
    static std::string httpGETRequest(const char* serverName);
    struct {
        std::string description;
        std::string icon;
        float feels_like = 0;
        float humidity = 0;
        float temp_max = 0;
        int pressure = 0;
        float temp_min = 0;
        float wind_speed = 0;
        float temp = 0;
    } weatherInfo;

};
