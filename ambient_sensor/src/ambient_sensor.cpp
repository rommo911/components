/**
 * @file rami_ambient_sensor.cpp
 * @author your name (you@domain.com)
 * @brief
 * @version 0.2
 * @date 2020-06-26
 *
 * @copyright Copyright (c) 2020
 *
 */

#include "ambient_sensor.h"
 /**
  * @brief Construct a new Light Sensor:: Light Sensor object & its config & its freeRtos Timer
  *
  * @param EventLoop
  */
LightSensor::LightSensor(homeassistant::BaseDevCtx& _hass_device, EventLoop_p_t& EventLoop, adc1_channel_t _channel, const char* _TAG):
Loop(EventLoop),
channel(_channel),
hass_device(_hass_device)
{
    timer = std::make_unique<ESPTimer>([&](void* arg) {this->TimerRun(); }, TAG);

}

LightSensor::~LightSensor()
{
    DeInit();
}

/**
 * @brief start all init and start task
 *
 * @return esp_err_t
 */
esp_err_t LightSensor::Init()
{
    if (isInitialized)
    {
        return ESP_OK;
    }

    if (lock == nullptr)
        lock = Semaphore::CreateUnique("LightSensor"); // create a semaphore
    HassLuxSensorDescovery = std::make_unique <homeassistant::SensorDiscovery>(hass_device, homeassistant::SensorDiscovery::illuminance, "lux");
    esp_err_t ret = adc1_config_width(ADC_WIDTH_BIT_12);
    ret = adc1_config_channel_atten(channel, ADC_ATTEN_DB_11);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "ADC LightSensor sensor Driver not Started");
        return ret;
    }
    meanValCounter = 0;
    meanValue.clear();
    for (int i = 0; i < smoothingFactor; i++)
    {
        meanValue.push_back(0); // put some weros into the fifo
    }
    timer->start_periodic(std::chrono::milliseconds(this->timerDelay));
    isInitialized = true;
    return ret;
}

/**
 * @brief start an ADC read , update variable table, private
 *
 * @return uint32_t
 */
uint32_t LightSensor::ReadADC()
{
    uint32_t adc_reading = 0;
    adc_reading = static_cast<uint32_t>(adc1_get_raw(channel));
    meanValue[meanValCounter] = adc_reading;
    meanValCounter++;
    if (meanValCounter >= smoothingFactor)
        meanValCounter = 0;
    uint32_t mean = 0;
    for (int i = 0; i < smoothingFactor; i++)
    {
        mean += meanValue[i];
    }
    lastSmoothedValue = mean / meanValue.size();
    for (const auto& target : targetVariables)
    {
        *target = lastSmoothedValue;
    }
    int diff = 0;
    if (lastSmoothedValue > adc_reading)
        diff = (lastSmoothedValue - adc_reading);
    else
    {
        diff = (adc_reading - lastSmoothedValue);
    }
    if (diff > (4096 / 100))
    {
        Loop->post_event_data(EVENT_CHANGED_1_PERC, diff);
    }
    if (diff > (4096 / 20))
    {
        Loop->post_event_data(EVENT_CHANGED_5_PERC, diff);
    }
    if (diff > (4096 / 10))
    {
        Loop->post_event_data(EVENT_CHANGED_10_PERC, diff);
    }
    if (diff > (4096 / 5))
    {
        Loop->post_event_data(EVENT_CHANGED_20_PERC, diff);
    }
    return adc_reading;
}

/**
 * @brief timer callback to be called each interval, private
 *          this will be called by the timer at each interval
 * @param xTimer
 */
esp_err_t LightSensor::TimerRun()
{
    lock->wait("ReadADC_Timer"); //TODO review
    lastValue = ReadADC();
    // Loop->post_event_data(EVENT_MEASURE, lastValue);
    return ESP_OK;
}

/**
 * @brief shut down all
 *
 */
void LightSensor::DeInit()
{
    lock->wait("DeInit", std::chrono::seconds(1));
    if (ESP_OK)
    {
        isInitialized = false;
        meanValue.clear();
        targetVariables.clear();
        this->timer->stop();
    }
    else
        ESP_LOGE(TAG, " something wenwrong while stopping timer");
}

/**
 * @brief register a variable pointers to be updated each interal ( up to 5 variables)
 *
 * @param var
 * @return esp_err_t
 */
esp_err_t LightSensor::RegisterVariable(uint32_t* var)
{
    if (var != NULL && isInitialized)
    {
        targetVariables.push_back(var);
        return ESP_OK;
    }
    return ESP_ERR_INVALID_ARG;
}

/**
 * @brief return last read value
 *
 * @return uint32_t
 */
uint32_t LightSensor::GetValue() const
{
    if (isInitialized)
    {
        return lastSmoothedValue;
    }
    else
    {
        return (0);
    }
}
 
LightSensor_t lightSensor = nullptr;