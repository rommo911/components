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
#include "string.h"
#include <chrono>
#include <cstring>
#include <map>
#include <memory>
#include <mutex>

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
    // just delay
    static void sleep(uint32_t ms);
    // start  a task and return status, including logging
    static esp_err_t StartTask(void task(void *), const char *taskName = "nan", uint32_t stackSize = 4096, void *param = NULL, UBaseType_t uxPriority = 2, TaskHandle_t *const pvCreatedTask = NULL, const BaseType_t xCoreID = 0);
    // return number freeRtos global tasks running
    static int GetRunningTaskNum();
    // return list of running task names
    static std::string GetTaskList();
    // get millis since system boot
    static uint32_t millis();

    // create and ESP Timer to run once ( not on freeRtos )
    class Timer
    {
    public:
        Timer(std::function<void()> fn, const char *name, bool _periodic) : m_TimerName(std::string(name)), periodic(_periodic)
        {
            if (fn == nullptr)
                std::__throw_invalid_argument("function pointer null");
            timerRunFn = fn;
            this->Create();
        }
        Timer(const char *name = "timer") : m_TimerName(std::string(name))
        {
        }
        Timer(const Timer& ) = default;
        Timer(Timer&& ) = default;
        ~Timer()
        {
            TimerDeinit();
        }
        esp_err_t SetFunction(std::function<void()> fn, bool _periodic);
        //
        bool TimerTaskIsRunning() const;
        //
        template <typename r, typename p>
        esp_err_t TimerStart(const std::chrono::duration<r, p> &timeout);
        esp_err_t TimerStart();
        esp_err_t TimerStop();
        template <typename r, typename p>
        esp_err_t TimerRestart(const std::chrono::duration<r, p> &timeout);
        esp_err_t TimerRestart();
        template <typename r, typename p>
        esp_err_t SetPeriod(const std::chrono::duration<r, p> &timeout);

    protected:
        esp_err_t TimerDeinit();

    private:
        std::function<void()> timerRunFn{nullptr};
        static void TimerStaticCB(TimerHandle_t arg);
        esp_err_t Create();
        TimerHandle_t m_TimerHandle = nullptr;
        TickType_t m_TimerTick = 0;
        const std::string m_TimerName;
        void *timerArgument = nullptr;
        std::timed_mutex lock;
        bool periodic{false};
        std::chrono::milliseconds period{0};
    };

    template <typename r, typename p>
    static TickType_t ToTicks(const std::chrono::duration<r, p> &time)
    {
        const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(time);
        return pdMS_TO_TICKS(ms.count());
    }
};

template <typename r, typename p>
esp_err_t FreeRTOS::Timer::TimerStart(const std::chrono::duration<r, p> &timeout)
{
    if (m_TimerHandle != nullptr)
    {
        if (!TimerTaskIsRunning())
        {
            this->SetPeriod(FreeRTOS::ToTicks(timeout));
            std::lock_guard<std::timed_mutex> __Lock(lock);
            BaseType_t ret = xTimerStart(m_TimerHandle, 0);
            return ret == pdPASS ? ESP_OK : ESP_FAIL;
        }
        else

        {
            return ESP_ERR_INVALID_STATE;
        }
    }
    return ESP_FAIL;
}
template <typename r, typename p>
esp_err_t FreeRTOS::Timer::TimerRestart(const std::chrono::duration<r, p> &timeout)
{
    if (m_TimerHandle != nullptr)
    {
        if (TimerTaskIsRunning())
            this->TimerStop();
        this->SetPeriod(FreeRTOS::ToTicks(timeout));
        std::lock_guard<std::timed_mutex> __Lock(lock);
        BaseType_t ret = xTimerStart(m_TimerHandle, 0);
        return ret == pdPASS ? ESP_OK : ESP_FAIL;
    }
    return ESP_FAIL;
}

template <typename r, typename p>
esp_err_t FreeRTOS::Timer::SetPeriod(const std::chrono::duration<r, p> &timeout)
{
    if (m_TimerHandle != nullptr)
    {
        std::lock_guard<std::timed_mutex> __Lock(lock);
        this->period = timeout;
        return xTimerChangePeriod(m_TimerHandle, FreeRTOS::ToTicks(period), 50) == pdPASS ? ESP_OK : ESP_FAIL;
    }
    return ESP_FAIL;
}

#endif // __FREERTOS_H__