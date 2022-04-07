/**
 * @file h
 * @author your name (you@domain.com)
 * @brief
 * @version 0.1
 * @date 2020-09-25
 *
 * @copyright Copyright (c) 2020
 *
 */
#pragma once
#ifndef _UTILITIES_H
#define _UTILITIES_H

#ifdef ESP_PLATFORM
#include "esp_system.h"
#include <esp_heap_caps.h>

#else
typedef int multi_heap_info_t;
#endif // CONFIG_HAL_MOCK

#include "esp_err.h"
#include "esp_log.h"
#include "stdarg.h"
#include "string.h"
#include <array>
#include <chrono>
#include <memory>
#include <regex>
#include <string>
#include <vector>
#include "esp_netif_ip_addr.h"

typedef std::array<uint8_t, 64> Array_64_uint8_t;
typedef std::array<uint8_t, 64> Array_64_uint16_t;
typedef std::array<uint16_t, 16> HTPA_ImageLine_t;
typedef std::array<uint16_t, 4> HTPA_ImageColumn_t;
typedef std::array<uint16_t, 64> HTPA_image_u16_64_t;
typedef std::array<int16_t, 64> HTPA_image_i16_64_t;
typedef HTPA_image_i16_64_t HTPA_absTempSubMeanFifo_t;

using namespace std::chrono;
#define GET_NOW_MILLIS std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count()
#define GET_NOW_SECONDS std::chrono::duration_cast<std::chrono::seconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count()
#define GET_NOW_MINUTES std::chrono::duration_cast<std::chrono::minutes>(std::chrono::high_resolution_clock::now().time_since_epoch()).count()

struct StrCompare
{
public:
    bool operator()(const char *str1, const char *str2) const
    {
        return std::strcmp(str1, str2) < 0;
    }
};

class tools
{
public:
    typedef enum
    {
        SKETCH_SIZE_TOTAL = 0,
        SKETCH_SIZE_FREE = 1
    } sketchSize_t;

    typedef enum
    {
        FM_QIO = 0x00,
        FM_QOUT = 0x01,
        FM_DIO = 0x02,
        FM_DOUT = 0x03,
        FM_FAST_READ = 0x04,
        FM_SLOW_READ = 0x05,
        FM_UNKNOWN = 0xff
    } FlashMode_t;
    struct TimeDate_t
    {
        uint8_t month, day, hour, minute, second;
        uint32_t year;
    };
    static TimeDate_t GetSystemTimeDate();
    static constexpr char TAG[] = "tools";
    static bool base64Decode(const std::string &in, std::string *out);
    static bool base64Encode(const std::string &in, std::string *out);
    static void dumpInfo();
    static bool endsWith(std::string str, char c);
    template <typename T>
    static void hexDump(const std::vector<T> &pData);
    static void hexDump(const uint8_t *pData, uint32_t length);
    static std::string ipToString(uint8_t *ip);
    static std::string ipToString(const esp_ip4_addr_t &ipadd);

    static std::vector<std::string> split(std::string source, char delimiter);
    static std::string toLower(std::string &value);
    static std::string trim(const std::string &str);
    static std::string dumpHeapInfo(uint8_t type = 0);
    static size_t getMinimumFreeHeapSize();
    static multi_heap_info_t GetHeapInfo(uint8_t type = 0);

    static std::string stringf(const std::string fmt_str, ...);

    static bool CompareBssid(const char bssid[18], const char bssid2[18]);
    static void macstr_to_uint(const char *str, uint8_t bssid[6]);

    static std::string MacToStr(const uint8_t mac[6], uint8_t count = 6, bool format = true);
    static bool IsValidUrl(const std::string &str);
    static void restart();
    // Internal RAM
    static uint32_t getHeapSize();     // total heap size
    static uint32_t getFreeHeap();     // available heap
    static uint32_t getMinFreeHeap();  // lowest level of free heap since boot
    static uint32_t getMaxAllocHeap(); // largest block of heap that can be allocated at once
                                       // SPI RAM
    static uint32_t getPsramSize();
    static uint32_t getFreePsram();
    static uint32_t getMinFreePsram();
    static uint32_t getMaxAllocPsram();
    // chip
    static uint8_t getChipRevision();
    static uint32_t getCpuFreqMHz();
#ifdef ESP_PLATFORM

    static inline uint32_t getCycleCount() __attribute__((always_inline));
    static const char *getSdkVersion();
    static void deepSleep(uint32_t time_us);
    // internal flash
    static uint32_t getFlashChipSize();
    static uint32_t getFlashChipSpeed();
    static FlashMode_t getFlashChipMode();
    static uint32_t magicFlashChipSize(uint8_t byte);
    static uint32_t magicFlashChipSpeed(uint8_t byte);
    static FlashMode_t magicFlashChipMode(uint8_t byte);
    static bool flashWrite(uint32_t offset, uint32_t *data, size_t size);
    static bool flashRead(uint32_t offset, uint32_t *data, size_t size);
    static bool flashEraseSector(uint32_t sector);

    // sketch
    static uint32_t getSketchSize();
    static std::string getSketchMD5();
    static std::string getOTASketchMD5();
    static uint32_t getFreeSketchSpace();
    static uint32_t sketchSize(sketchSize_t response);
    // EFUSE
    static uint64_t getEfuseMac();
#endif
    // math
    static long random(long howbig);
    static void randomSeed(unsigned long);
    static long map(long, long, long, long, long);
    static long random(long howsmall, long howbig);
    static unsigned int makeWord(unsigned char h, unsigned char l);
    // char manipulation
    static int atoi(const char *s);
    static long atol(const char *s);
    static double atof(const char *s);
    static char *itoa(int val, char *s, int radix);
    static char *ltoa(long val, char *s, int radix);
    static char *utoa(unsigned int val, char *s, int radix);
    static char *ultoa(unsigned long val, char *s, int radix);
    static char *dtostrf(double val, signed char width, unsigned char prec, char *s);
    static void reverse(char *begin, char *end);
    template <typename InputIt, typename SourceLen, typename T, std::size_t size>
    static void copy_min_to_buffer(InputIt source, SourceLen source_length, T (&target)[size]);
};

#ifdef ESP_PLATFORM

#endif // #ifdef ESP_PLATFORM

static inline unsigned long long operator"" _kHz(unsigned long long x)
{
    return x * 1000;
}

static inline unsigned long long operator"" _MHz(unsigned long long x)
{
    return x * 1000 * 1000;
}

static inline unsigned long long operator"" _kB(unsigned long long x)
{
    return x * 1024;
}

static inline unsigned long long operator"" _MB(unsigned long long x)
{
    return x * 1024 * 1024;
}

uint32_t IRAM_ATTR tools::getCycleCount()
{
    uint32_t ccount;
    __asm__ __volatile__("esync; rsr %0,ccount"
                         : "=a"(ccount));
    return ccount;
}

template <typename InputIt, typename SourceLen, typename T, std::size_t size>
void tools::copy_min_to_buffer(InputIt source, SourceLen source_length, T (&target)[size])
{
    auto to_copy = std::min(static_cast<std::size_t>(source_length), size);
    std::copy_n(source, to_copy, target);
}

template <typename T>
void tools::hexDump(const std::vector<T> &pData)
{
    char ascii[80];
    char hex[80];
    char tempBuf[80];
    uint32_t lineNumber = 0;
    strcpy(ascii, "");
    strcpy(hex, "");
    uint32_t index = 0;
    while (index < pData.size())
    {
        int Tsize = sizeof(T);
        std::to_string(Tsize);
        switch (Tsize)
        {
        case 1:
            sprintf(tempBuf, "%.2x ", pData[index]);
            break;
        case 2:
            sprintf(tempBuf, "%.4x ", pData[index]);
            break;
        case 4:
            sprintf(tempBuf, "%.6x ", pData[index]);
            break;
        case 8:
            sprintf(tempBuf, "%.8x ", pData[index]);
            break;
        default:
            break;
        }
        strcat(hex, tempBuf);
        if (isprint(pData[index]))
        {
            sprintf(tempBuf, "%c", pData[index]);
        }
        else
        {
            sprintf(tempBuf, ".");
        }
        strcat(ascii, tempBuf);
        index++;
        if (index % 16 == 0)
        {
            ESP_LOGI(TAG, "%.4x %s %s", lineNumber * 16, hex, ascii);
            strcpy(ascii, "");
            strcpy(hex, "");
            lineNumber++;
        }
    }
    if (index % 16 != 0)
    {
        while (index % 16 != 0)
        {
            strcat(hex, "   ");
            index++;
        }
        ESP_LOGI(TAG, "%.4x %s %s", lineNumber * 16, hex, ascii);
    }
}

#endif // #if _UTILITIES_H
