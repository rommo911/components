#ifndef __SYSTEM_H__
#define __SYSTEM_H__
#pragma once

#include "config.hpp"
#include "utilities.hpp"
#include "esp_event_cxx.hpp"
#include "esp_exception.hpp"
#include "esp_timer_cxx.hpp"
#include <nlohmann/json.hpp>
#include <optional>
using json = nlohmann::json;
using namespace std::chrono;
using namespace std::chrono_literals;
typedef void (*FreeRTOSTimerCb)(void *arg);
#define PACKED __attribute__((packed))
typedef idf::event::ESPEventLoop EventLoop;
typedef std::shared_ptr<EventLoop> EventLoop_p_t;
typedef idf::event::ESPEvent Event_t;
typedef idf::event::ESPEventID EventID_t;
typedef idf::event::ESPEventHandlerSync EventHandlerSync_t;
typedef std::unique_ptr<idf::event::ESPEventReg> EventHandlerReg_t;
typedef uint8_t mac_addr_t[6];

struct CustomError_t
{
    uint32_t ts;
    esp_err_t esp_err;
    std::string errorStr;
    CustomError_t() = delete;
    CustomError_t(const esp_err_t _err, const char *__TAG = "") : esp_err(_err)
    {
        ts = GET_NOW_MILLIS;
        errorStr = __TAG;
        errorStr += (esp_err_to_name(_err));
    }
};
typedef std::optional<CustomError_t> ErrorOptional_t;
typedef CustomError_t OptionalError_t;

#endif // __SYSTEM_H__