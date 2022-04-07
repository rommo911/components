
#include "sdkconfig.h"
#include "string.h"
#ifdef ESP_PLATFORM
#include "esp32/spiram.h"
#include "esp_attr.h"
#include "esp_heap_caps.h"
#include "freertos/FreeRTOS.h"
#else
#include "FreeRTOS.h"
#include <stdbool.h>
#endif
static volatile bool spiramDetected = false;
static volatile bool spiramFailed = false;
#ifdef ESP_PLATFORM

void *psRamMalloc(size_t size)
{
#if CONFIG_SPIRAM

    if (esp_spiram_is_initialized())
        return heap_caps_malloc(size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    else
#endif
        return malloc(size);
}

void *psRamCalloc(size_t n, size_t size)
{
#if CONFIG_SPIRAM
    if (esp_spiram_is_initialized())
        return heap_caps_calloc(n, size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    else
#endif
        return calloc(n, size);
}

void *psRamRealloc(void *ptr, size_t size)
{
#if CONFIG_SPIRAM

    if (esp_spiram_is_initialized())
        return heap_caps_realloc(ptr, size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    else
#endif
        return realloc(ptr, size);
}
bool psramFound()
{
    return esp_spiram_is_initialized();
}
#else  //CONFIG_HAL_MOCK

/*-------------------------- MOCK SECTION --------------------------*/

void *psRamMalloc(size_t size)
{
    return pvPortMalloc(size);
    //warning ?
}

void *psRamCalloc(size_t n, size_t size)
{
    void *ret = pvPortMalloc(size * n);
    if (ret != NULL)
        memset(ret, 0, size * n);
    return ret;
    //warning ?
}

void *psRamRealloc(void *ptr, size_t size)
{
   return pvPortMalloc(size);
}

bool psramFound()
{
    return true;
}
#endif //CONFIG_HAL_MOCK