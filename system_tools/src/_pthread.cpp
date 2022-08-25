#include "_pthread.hpp"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
std::mutex AladinPthread_t::_mutex;

#ifdef ESP_PLATFORM
esp_pthread_cfg_t AladinPthread_t::CreateConfig(const char *name, int core_id, int stack, int prio)
{
    esp_pthread_cfg_t cfg = {
        .stack_size = static_cast<std::size_t>(stack),
        .prio = static_cast<std::size_t>(prio),
        .inherit_cfg = false,
        .thread_name = name,
        .pin_to_core = core_id,
    };
    return cfg;
}
#endif

void AladinPthread_t::Join()
{
    if (_thread.joinable())
        this->_thread.join();
}

uint32_t AladinPthread_t::GetStackWaterMark()
{
    return uxTaskGetStackHighWaterMark(xTaskGetCurrentTaskHandle());
}
