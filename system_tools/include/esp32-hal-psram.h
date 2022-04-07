

/**
 * @file esp32-hal-psram.h
 * @author your name (you@domain.com)
 * @brief 
 * @version 0.1
 * @date 2021-01-09
 * 
 * @copyright Copyright (c) 2021
 * 
 */

#ifndef _ESP32_HAL_PSRAM_H_
#define _ESP32_HAL_PSRAM_H_
#pragma once

#include "stdint.h"

#ifdef __cplusplus
extern "C"
{
#endif

    bool  psramFound();
    void * psRamMalloc(size_t size);
    void * psRamCalloc(size_t n, size_t size);
    void * psRamRealloc(void *ptr, size_t size);

#ifdef __cplusplus
}
#endif

#endif /* _ESP32_HAL_PSRAM_H_ */
