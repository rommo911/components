#ifndef __FREERTOS2_H__
#define __FREERTOS2_H__
#pragma once

#include "sdkconfig.h"
#ifdef ESP_PLATFORM 
#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>
#include <freertos/semphr.h>
#include <freertos/task.h>
#include <freertos/timers.h>
#else 
#include <FreeRTOS.h>
#include <event_groups.h>
#include <semphr.h>
#include <task.h>
#include <timers.h>
#define tskNO_AFFINITY 0
#endif
#include "esp_err.h"
#include "semphr.hpp"
#include "string.h"
#include <chrono>
#include <cstring>
#include <map>
#include <memory>
/**
 * @file FreeRTOS.h
 * @author your name (you@domain.com)
 * @brief 
 * @version 0.1
 * @date 2021-01-22
 * 
 * @copyright Copyright (c) 2021
 * 
 */

class FreeRTOS
{

private:
    struct StrCompare 
    {
    public:
        bool operator()(const char *str1, const char *str2) const
        {
            return std::strcmp(str1, str2) == 0;
        }
    };

public:
    //just delay
    static void sleep(uint32_t ms);
    //start  a task and return status, including logging
    static esp_err_t StartTask(void task(void *), const char *taskName = "nan", uint32_t stackSize = 3100, void *param = NULL, UBaseType_t uxPriority = 2, TaskHandle_t *const pvCreatedTask = NULL, const BaseType_t xCoreID = 0);
    //return number freeRtos global tasks running
    static int GetRunningTaskNum();
    // return list of running task names
    static std::string GetTaskList();
    //get millis since system boot
    static uint32_t millis();

    //create and ESP Timer to run once ( not on freeRtos )
    class Timer
    {
    public:
        Timer(const char *name, bool Periodic) : m_TimerName(std::string(name))
        {
            lock = std::make_unique<Semaphore>(name);
            Create(Periodic);
        }
        ~Timer()
        {
            TimerDeinit();
        }
        virtual esp_err_t TimerRun(void *arg) = 0;
        //

    protected:
        esp_err_t TimerModify(const uint32_t millisSeconds);
        //deinit timer and delete its Handle
        esp_err_t TimerDeinit();
        //
        bool TimerTaskIsRunning() const;
        //
        esp_err_t TimerStart(std::chrono::milliseconds ms, void *arg = nullptr);

        esp_err_t TimerStop();

    private:
        static void TimerStaticCB(TimerHandle_t arg);
        esp_err_t Create(const bool Periodic);
        static std::map<const char *, TimerHandle_t, StrCompare> runningTimersList;
        TimerHandle_t m_TimerHandle = nullptr;
        TickType_t m_TimerTick = 0;
        std::string m_TimerName = "";
        void *timerArgument = nullptr;
        std::unique_ptr<Semaphore> lock;
    };
    template <typename r, typename p>
    static TickType_t ToTicks(const std::chrono::duration<r, p> &time)
    {
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(time);
        return pdMS_TO_TICKS(ms.count());
    }
    template <typename r, typename p>
    static void Delay(const std::chrono::duration<r, p> &time)
    {
        vTaskDelay(ToTicks(time));
    }
};

#endif // __FREERTOS_H__