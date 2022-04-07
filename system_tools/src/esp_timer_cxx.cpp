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

#ifdef __cpp_exceptions

#include "sdkconfig.h"
#ifdef ESP_PLATFORM
#include "esp_exception.hpp"
#include "esp_timer_cxx.hpp"
#include <functional>
#include "esp_log.h"

ESPTimer::ESPTimer(std::function<void(void* arg)> timeout_cb, const std::string& timer_name)
    : timeout_cb(timeout_cb), name(timer_name)
{
    esp_timer_create_args_t timer_args = {};
    timer_args.callback = esp_timer_cb;
    timer_args.arg = this;
    timer_args.dispatch_method = ESP_TIMER_TASK;
    timer_args.name = name.c_str();
    esp_timer_create(&timer_args, &timer_handle);
}

ESPTimer::~ESPTimer()
{
    // Ignore potential ESP_ERR_INVALID_STATE here to not throw exception.
    esp_timer_stop(timer_handle);
    esp_timer_delete(timer_handle);
}

/**
    * @brief Set the Callback function
    *
    * @param _timeout_cb
    */
void ESPTimer::setCallback(std::function<void(void*)> _timeout_cb) noexcept
{
    if (_timeout_cb != nullptr && this->currentType == NONE)
    {
        this->timeout_cb = _timeout_cb;
    }
}

/**
    * @brief Stop the previously started timer.
    *
    * This function stops the timer previously started using \c start() or \c start_periodic().
    *
    * @throws ESPException with error ESP_ERR_INVALID_STATE if the timer has not been started yet.
    */
esp_err_t ESPTimer::stop() noexcept
{
    std::lock_guard<std::mutex> guard(lock);
    esp_err_t ret = currentType == NONE ? ESP_OK : esp_timer_stop(timer_handle);
    if (ret == ESP_OK)
    {
        this->currentType = NONE;
    }
    return ret;
}

/**
 * @brief Start periodic timer
 *
 * Timer could be running or not when this function is called. This function will
 * stop a timer, and restart it which will trigger every 'period' microseconds.
 *
 *
 * @param arg
 *  @return esp_err_t
 */
esp_err_t ESPTimer::reset_periodic(void* arg)
{
    if (timeout_cb == nullptr)
    {
        return ESP_ERR_INVALID_STATE;
    }
    if (stop() == ESP_OK)
    {
        if (arg != nullptr)
        {
            m_arg = arg;
        }
        return start_periodic(this->period, m_arg);
    }
    return ESP_FAIL;
}

/**
 * @brief Timer could be running or not when this function is called. This function will
 * stop a timer if was not yet triggered, and restart it which will triggerd once in a 'period' microseconds.
 *
 * @param arg
 * @return esp_err_t
 */
esp_err_t ESPTimer::reset_once(void* arg) {
    if (stop() == ESP_OK)
    {
        if (arg != nullptr)
        {
            m_arg = arg;
        }
        return start_once(this->period, this->m_arg);
    }
    return ESP_FAIL;
}


void ESPTimer::esp_timer_cb(void* arg)
{
    ESPTimer* timer = static_cast<ESPTimer*>(arg);
    if (timer->timeout_cb != nullptr)
    {
        ESP_LOGV(timer->name.c_str(), "Timer EXPIRED, executing cb");
        timer->timeout_cb(timer->m_arg);
    }
    if (timer->currentType == ONCE)
    {
        timer->currentType = NONE;
    }
    if (timer->autoDestroy)
    {
        timer->~ESPTimer();
    }
}

#endif // CONFIG_HAL_MOCK
#endif