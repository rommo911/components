/**
 * @file MD5Builder2.h
 * @author your name (you@domain.com)
 * @brief 
 * @version 0.1
 * @date 2021-01-09
 * 
 * @copyright Copyright (c) 2021
 * 
 */

#ifndef MD5_BUILDER_H
#define MD5_BUILDER_H
#include "sdkconfig.h"
#ifndef CONFIG_HAL_MOCK 
#include "esp_rom_md5.h"
#include "esp32/rom/crc.h"
#include "sodium.h"
#else
#include <openssl/md5.h>
typedef MD5_CTX MD5Context;
#endif

#include "esp_err.h"
#include "string.h"
#include <sstream>
#include <string>
#include <vector>
class MD5Builder2
{
private:
    MD5Context _ctx;
    uint8_t _buf[16];

public:
    void Begin(void);
    void Add(uint8_t *data, uint16_t len);
    void Add(const char *data)
    {
        Add((uint8_t *)data, strlen(data));
    }
    void Add(char *data)
    {
        Add((const char *)data);
    }
    void Add(std::string data)
    {
        Add(data.c_str());
    }
    void AddHexString(const char *data);
    void AddHexString(char *data)
    {
        AddHexString((const char *)data);
    }
    void AddHexString(const std::string &data)
    {
        AddHexString(data.c_str());
    }
    void Calculate(void);
    void GetBytes(uint8_t *output);
    void GetChars(char *output);
    std::string to_String(void);
};
#define CONFIG_HASHING_ENABLED
#ifdef CONFIG_HASHING_ENABLED

class HashBuilder_t
{
private:
    uint8_t *_buf = nullptr;
    std::stringstream str;
    size_t hashSize = crypto_generichash_BYTES;

public:
    explicit HashBuilder_t(size_t _HashSize = crypto_generichash_BYTES) : hashSize(_HashSize)
    {
        _buf = (uint8_t *)malloc(sizeof(uint8_t) * _HashSize);
    }
    ~HashBuilder_t()
    {
        free(_buf);
    }
    template <typename T>
    esp_err_t add(T data);
    const size_t &size() { return hashSize; }
    esp_err_t calculateHashCrypto();
    uint32_t calculateCRC32();
    std::string String(void);
    const char *c_str(void);
    const uint8_t *data(void);
    static std::vector<uint8_t> FastGenerateHash(const char *_str);
    //static uint32_t FastGenerateCRC32(const char *_str);
};

template <typename T>
esp_err_t HashBuilder_t::add(T data)
{
    try
    {
        str << data;
        return ESP_OK;
    }
    catch (const std::exception &e)
    {
    }
    return ESP_FAIL;
}
#endif

#endif // __MD5BUILDER_H__