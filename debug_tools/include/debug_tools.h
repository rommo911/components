/**
 * @file rami_debug.h
 * @author your name (you@domain.com)
 * @brief 
 * @version 0.1
 * @date 2020-09-25
 * 
 * @copyright Copyright (c) 2020
 * 
 */
#ifndef rami_DEBUG_H
#define rami_DEBUG_H
#define VERBOS 2
#define INFO 1
#define LOG_NONE 0
//////////////ACTIVATE LOGGGING LEVEL ///////////
//#define DEBUG_AMBIENT VERBOS
//#define DEBUG_BUZZER INFO
//#define DEBUG_NTP VERBOS
//#define DEBUG_STORAGE INFO
#define DEBUG_MQTT INFO
//#define DEBUG_WEBCLIENT INFO
//#define DEBUG_WIFI_DIRECTORY INFO
#ifdef ESP_PLATFORM
//logging redirection //
#define LOGC(TAG, ...)          \
    ESP_LOGE(TAG, __VA_ARGS__); \
    // ramiLogger.CreatLog(Critical, TAG, __VA_ARGS__);

#define LOGE(TAG, ...)          \
    ESP_LOGE(TAG, __VA_ARGS__); \
    // ramiLogger.CreatLog(Error, TAG, __VA_ARGS__);

#define LOGW(TAG, ...)          \
    ESP_LOGW(TAG, __VA_ARGS__); \
    //ramiLogger.CreatLog(Warning, TAG, __VA_ARGS__);

#define LOGI(TAG, ...)          \
    ESP_LOGI(TAG, __VA_ARGS__); \
    // ramiLogger.CreatLog(Info, TAG, __VA_ARGS__);

#define LOGD(TAG, ...)            \
    ;                             \
    /*ESP_LOGD(TAG, __VA_ARGS__); \
    ramiLogger.CreatLog(Debug, TAG, __VA_ARGS__);*/

#define LOGV(TAG, ...)             \
    ;                              \
    /* ESP_LOGV(TAG, __VA_ARGS__); \
     ramiLogger.CreatLog(Verbos, TAG, __VA_ARGS__);*/
#else
#include <iostream>
#include <stdio.h>
#include <string>
template<typename ... Args>
std::string string_format(const std::string& format, Args ... args)
{
    int size_s = std::snprintf(nullptr, 0, format.c_str(), args ...) + 1; // Extra space for '\0'
    if (size_s <= 0) { throw std::runtime_error("Error during formatting."); }
    auto size = static_cast<size_t>(size_s);
    auto buf = std::make_unique<char[]>(size);
    std::snprintf(buf.get(), size, format.c_str(), args ...);
    return std::string(buf.get(), buf.get() + size - 1); // We don't want the '\0' inside
}
#define LOGE(tag,s,...) std::string tt=string_format(s, __VA_ARGS__ ); std::cout<<"Error-"<<tag<<":"<<tt;
#define LOGC(tag,s,...) std::string tt=string_format(s, __VA_ARGS__ ); std::cout<< "Critical-" <<tag<<":"<<tt;
#define LOGW(tag,s,...) std::string tt=string_format(s, __VA_ARGS__ ); std::cout<< "Warning-" <<tag<<":"<<tt;
#define LOGI(tag,s,...) std::string tt=string_format(s, __VA_ARGS__ ); std::cout<< "Info-" <<tag<<":"<<tt;
#define LOGD(tag,s,...) std::string tt=string_format(s, __VA_ARGS__ ); std::cout<< "Debug-" <<tag<<":"<<tt;

#endif 

#endif
