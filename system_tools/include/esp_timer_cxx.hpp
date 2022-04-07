// Copyright 2020 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//         http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once

#ifdef __cpp_exceptions

#include "esp_exception.hpp"
#include <chrono>
#include <functional>
#include <memory>
#ifdef ESP_PLATFORM

#include "esp_timer.h"
#include <mutex>
const std::chrono::microseconds zeroMicroSeconds(0);
/**
 * @brief Get time since boot
 * @return time since \c esp_timer_init() was called (this normally happens early during application startup).
 */
static inline std::chrono::microseconds get_time()
{
    return std::chrono::microseconds(esp_timer_get_time());
}

/**
 * @brief Get the timestamp when the next timeout is expected to occur
 * @return Timestamp of the nearest timer event.
 *         The timebase is the same as for the values returned by \c get_time().
 */
static inline std::chrono::microseconds get_next_alarm()
{
    return std::chrono::microseconds(esp_timer_get_next_alarm());
}

/**
 * @brief
 * A timer using the esp_timer component which can be started either as one-shot timer or periodically.
 */
class ESPTimer
{
public:
    enum TimerType
    {
        NONE = -1,
        PERIODIC = 0,
        ONCE = 1,
    };
    /**
     * @param timeout_cb The timeout callback.
     * @param timer_name The name of the timer (optional). This is for debugging using \c esp_timer_dump().
     */
    ESPTimer(std::function<void(void*)> timeout_cb = nullptr, const std::string& timer_name = "ESPTimer");
    /**
     * Stop the timer if necessary and delete it.
     */
    ~ESPTimer();
    /**
     * Default copy constructor is deleted since one instance of esp_timer_handle_t must not be shared.
     */
    ESPTimer(const ESPTimer&) = delete;
    /**
     * Default copy assignment is deleted since one instance of esp_timer_handle_t must not be shared.
     */
    ESPTimer& operator=(const ESPTimer&) = delete;
    /**
     * @brief Start one-shot timer
     *
     * Timer should not be running (started) when this function is called.
     *
     * @param timeout timer timeout, in microseconds relative to the current moment.
     *
     * @throws ESPException with error ESP_ERR_INVALID_STATE if the timer is already running.
     */
    template <typename r, typename p>
    esp_err_t start_once(const std::chrono::duration<r, p>& timeout, void* arg = nullptr) noexcept
    {
        if (timeout_cb == nullptr)
        {
            return ESP_ERR_INVALID_STATE;
        }
        std::lock_guard<std::mutex> guard(lock);
        m_arg = arg;
        if (period != timeout)
        {
            setPeriod(timeout);
        }
        esp_err_t ret = esp_timer_start_once(timer_handle, period.count());
        if (ret == ESP_OK)
        {
            currentType = ONCE;
        }
        return ret;
    }

    /**
     * @brief Set the Period time
     *
     * @tparam r std::chrono
     * @tparam p std::chrono
     * @param timeout
     */
    template <typename r, typename p>
    void setPeriod(const std::chrono::duration<r, p>& timeout) noexcept
    {
        period = std::chrono::duration_cast<std::chrono::microseconds>(timeout);
    }


    /**
     * @brief Start periodic timer
     *
     * Timer should not be running when this function is called. This function will
     * start a timer which will trigger every 'period' microseconds.
     *
     * Timer should not be running (started) when this function is called.
     *
     * @param timeout timer timeout, in microseconds relative to the current moment.
     */
    template <typename r, typename p>
    esp_err_t start_periodic(const std::chrono::duration<r, p>& _period, void* arg = nullptr) noexcept
    {
        if (timeout_cb == nullptr)
        {
            return ESP_ERR_INVALID_STATE;
        }
        std::lock_guard<std::mutex> guard(lock);
        m_arg = arg;
        period = std::chrono::duration_cast<std::chrono::microseconds>(_period);
        esp_err_t ret = esp_timer_start_periodic(timer_handle, period.count());
        if (ret == ESP_OK)
        {
            currentType = PERIODIC;
        }
        return ret;
    }


    template <typename r, typename p>
    static auto FastTimerOnce(std::function<void(void*)> timeout_cb, const std::chrono::duration<r, p>& time, const std::string& timer_name = "ESPTimer", void* arg = nullptr, bool auto_destroy = false)
    {
        auto timer = std::make_shared<ESPTimer>(timeout_cb, timer_name);
        timer->autoDestroy = auto_destroy;
        timer->start_once(std::chrono::duration_cast<std::chrono::microseconds>(time), arg);
        return timer;
    }
    esp_err_t stop() noexcept;
    esp_err_t reset_periodic(void* arg = nullptr);
    esp_err_t reset_once(void* arg = nullptr);
    void setCallback(std::function<void(void*)> _timeout_cb) noexcept;

    const auto& GetPeriod()
    {
        return period;
    }

private:
    /**
     * Internal callback to hook into esp_timer component.
     */
    static void esp_timer_cb(void* arg);
    /**
     * Timer instance of the underlying esp_event component.
     */
    esp_timer_handle_t timer_handle;
    /**
     * Callback which will be called once the timer triggers.
     */
    std::function<void(void*)> timeout_cb;
    void* m_arg;
    /**
     * Name of the timer, will be passed to the underlying timer framework and is used for debugging.
     */
    const std::string name;
    /**
     * expiration timeout
     */
    std::chrono::microseconds period{ 0 };
    /**
     * experimental
     */
    bool autoDestroy{ false };
    /**
     * flag
     */
    TimerType currentType = NONE;
    std::mutex lock;
};
typedef std::unique_ptr<ESPTimer> ESPTimer_p_t;
#endif
#endif // __cpp_exceptions
