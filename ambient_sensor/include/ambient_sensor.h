/**
 * @file rami_ambient_sensor.h
 * @author your name (you@domain.com)
 * @brief
 * @version 0.2
 * @date 2020-06-26
 *
 * @copyright Copyright (c) 2020
 *
 */
#ifndef rami_AMBIENT_SENSOR_H
#define rami_AMBIENT_SENSOR_H
#pragma once

#include "FreeRTOS.hpp"
#include "config.hpp"
#include "system.hpp"
#include "driver/adc_common.h"
#include "stdlib.h"
#include <atomic>
#include <list>
#include <memory>
#include <vector>
#include "homeassistant.h"

class LightSensor
{
private:
    EventLoop_p_t Loop;
    bool isInitialized = false;
    unsigned int timerDelay = CONFIG_AMBIENT_SENSOR_DEFAULT_DELAY;
    uint8_t smoothingFactor = CONFIG_AMBIENT_SENSOR_DEFAULT_SMOOTHING_FACTOR; //size of vector used to smooth out the value
    unsigned int lastSmoothedValue = 0;                                       // actual last smoothed value to be passed to the variables
    unsigned int lastValue = 0;                                               // actual last raw value rad in ADC
    std::vector<uint32_t> meanValue;                                          // containning last several measures to smoothe out and put tot the lastSmoothedValue
    uint8_t meanValCounter;                                                   // used to manage the meanValue vector as a FIFO
    std::list<unsigned int*> targetVariables; // list of pointers to variable to be updated with last value - use
    static constexpr char TAG[] = "light_sensor";
    adc1_channel_t channel;
    ESPTimer_p_t timer{ nullptr };

    //EVENTS

public:
    const Event_t EVENT_CHANGED_1_PERC = { TAG, EventID_t(0) };
    const Event_t EVENT_CHANGED_5_PERC = { TAG, EventID_t(3) };
    const Event_t EVENT_CHANGED_10_PERC = { TAG, EventID_t(1) };
    const Event_t EVENT_CHANGED_20_PERC = { TAG, EventID_t(2) };
    const Event_t EVENT_MEASURE = { TAG, EventID_t(3) };
    //
    LightSensor(EventLoop_p_t& EventLoop, adc1_channel_t _channel, const char* _TAG);
    ~LightSensor();
    //register a pointer to be updated automatically with the ambient light value
    //uint32_t *var ( up to 5 pointers are allowed)
    esp_err_t RegisterVariable(uint32_t* var);
    //
    EventGroupHandle_t Event();
    //initialize ADC converter to read Albient light intensity
    //this will creat a timer that updates the values at a constant interval
    esp_err_t Init();
    //
    esp_err_t RegisterVariable(std::atomic<uint32_t>* var);
    //Get Lastest value read from ADC
    uint32_t GetValue() const;
    // shut down the ADC and delete the task
    void DeInit();
    // attach to mqtt //TODO
    esp_err_t Diagnose();

private:
    uint32_t ReadADC(); // low level read
    esp_err_t SetSmoothingFactor(uint8_t val);

    //CONFIG OVERRIDE
    esp_err_t TimerRun(); // actual measure on timer trigger // stativ and need to get "this" pointer
};
#endif