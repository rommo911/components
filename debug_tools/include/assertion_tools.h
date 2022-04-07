/**
 * @file rami_assertions.h
 * @author your name (you@domain.com)
 * @brief 
 * @version 0.1
 * @date 2020-10-26
 * 
 * @copyright Copyright (c) 2020
 * 
 */

#ifndef rami_ASSERT_H
#define rami_ASSERT_H
#ifdef ESP_PLATFORM
#include "esp_log.h"
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
#endif 
/*
ASSERTION DEFINES 
*/
////////////////////NULL /////////////////////
#define AssertNull_RETURN_NULL(pointer, TAG)                              \
    if (pointer == NULL)                                                  \
    {                                                                     \
        ESP_LOGE(TAG, "Error creating Json instance ! Maybe no free memory"); \
        return NULL;                                                      \
    }

#define AssertNull_RETURN_ERR(pointer, TAG)        \
    if (pointer == NULL)                           \
    {                                              \
        ESP_LOGE(TAG, "Error Argument passed i null"); \
        return ESP_FAIL;                           \
    }
#define ASSERT_RET_WITH_MSG_RETURN_RET(ret, str) \
    if (ret != ESP_OK)                           \
    {                                            \
        ESP_LOGE(TAG, str);                          \
        return ret;                              \
    }
///////////////////NULL/////////////////////////

//////////////////INIT//////////////////////////
#define ASSERT_INIT_AND_RETURN_ERR(init, TAG)               \
    {                                                       \
        if (!init)                                          \
        {                                                   \
            ESP_LOGE(TAG, " NOT initialized ! returning "); \
            return ESP_ERR_INVALID_STATE;                   \
        }                                                   \
    }

#define ASSERT_INIT_RETURN_NULL(init, TAG)            \
    if (!init)                                        \
    {                                                 \
        ESP_LOGE(TAG, "NOT initialized or failed ");  \
        return NULL;                                  \
    }

#define ASSERT_INIT_RETURN_ZERO(init, TAG)            \
    if (!init)                                        \
    {                                                 \
        ESP_LOGE(TAG, "NOT initialized or failed ");  \
        return 0;                                     \
    }

#define ASSERT_INIT_NO_RETURN(init, TAG)              \
    if (!init)                                        \
    {                                                 \
        ESP_LOGE(TAG, "NOT initialized or failed ");      \
        return;                                       \
    }
//////////////////INIT//////////////////////////

////////////////  esp_err_t //////////////////
#define ASSERT_ERR_WITH_MSG_CODE_AND_RETRUN_RET(str, ret) \
    if (ret != ESP_OK)                                    \
    {                                                     \
        ESP_LOGE(TAG, str, esp_err_to_name(ret));             \
        return ret;                                       \
    }

#define ASSERT_TRUE_WITH_MSG_AND_RETRUN_FAIL(cond, msg) \
    if (cond != true)                                   \
    {                                                   \
        ESP_LOGE(TAG, msg);                                 \
        return ESP_FAIL;                                \
    }
////////////////  esp_err_t //////////////////

#define SPI_TRANSACTION_ERROR_ASSERT                                   \
    if (ret != ESP_OK)                                                 \
    {                                                                  \
        ESP_LOGE(TAG, "SPI Transcation error %s :", esp_err_to_name(ret)); \
    }
#define ASSERT_ERR_RAISE_HW_ERR_FLAG(ret, rgb) \
    if (ret != ESP_OK)                         \
    {                                          \
        rgb->SetFlagHwAnomaly(true);           \
    }

/////////////////ASSET NUMBER no return  ////////////////
/*
#define ASSERT_PERCENTAGE(perc)               \
    if (perc < 1 || perc > 100)               \
    {                                         \
        LOGE(TAG, "percentage assert error"); \
        return;                               \
    }
    */

#define ASSERT_TIME_RETURN(time)                        \
    if (time < 1 || time > 1000000)                     \
    {                                                   \
        return;                                         \
    }

//////////////////ASSERT CHAR //////
#define ASSERT_STRLEN_RET_ERR(char)                     \
    if (!strlen(char))                                  \
    {                                                   \
        return ESP_ERR_INVALID_ARG;                     \
    }
///////////misc //////////////////////

#define BYTE_TO_BINARY_PATTERN "%c%c%c%c%c%c%c%c :"
#define BYTE_TO_BINARY(byte)       \
    (byte & 0x80 ? '1' : '0'),     \
        (byte & 0x40 ? '1' : '0'), \
        (byte & 0x20 ? '1' : '0'), \
        (byte & 0x10 ? '1' : '0'), \
        (byte & 0x08 ? '1' : '0'), \
        (byte & 0x04 ? '1' : '0'), \
        (byte & 0x02 ? '1' : '0'), \
        (byte & 0x01 ? '1' : '0')

#endif
