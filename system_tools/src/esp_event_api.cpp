#include "sdkconfig.h"
#ifdef ESP_PLATFORM
#include "utilities.hpp"
#include "esp_event.h"
#include "esp_event_api.hpp"
#include "esp_event_cxx.hpp"
#ifdef __cpp_exceptions

namespace idf
{

    namespace event
    {

        ESPEventAPIDefault::ESPEventAPIDefault()
        {
            esp_err_t res = esp_event_loop_create_default();
            if (res != ESP_OK && res != ESP_ERR_INVALID_STATE)
            {
                throw idf::event::EventException(res);
            }
        }

        ESPEventAPIDefault::~ESPEventAPIDefault()
        {
            esp_event_loop_delete_default();
        }

        esp_err_t ESPEventAPIDefault::handler_register(esp_event_base_t event_base,
                                                       int32_t event_id,
                                                       esp_event_handler_t event_handler,
                                                       void *event_handler_arg,
                                                       esp_event_handler_instance_t *instance)
        {
            return esp_event_handler_instance_register(event_base,
                                                       event_id,
                                                       event_handler,
                                                       event_handler_arg,
                                                       instance);
        }

        esp_err_t ESPEventAPIDefault::handler_unregister(esp_event_base_t event_base,
                                                         int32_t event_id,
                                                         esp_event_handler_instance_t instance)
        {
            return esp_event_handler_instance_unregister(event_base, event_id, instance);
        }

        esp_err_t ESPEventAPIDefault::post(esp_event_base_t event_base,
                                           int32_t event_id,
                                           void *event_data,
                                           size_t event_data_size,
                                           TickType_t ticks_to_wait)
        {
            return esp_event_post(event_base,
                                  event_id,
                                  event_data,
                                  event_data_size,
                                  ticks_to_wait);
        }
#if CONFIG_ESP_EVENT_POST_FROM_ISR
        esp_err_t ESPEventAPIDefault::post_from_isr(esp_event_base_t event_base,
                                                    int32_t event_id,
                                                    void *event_data,
                                                    size_t event_data_size)
        {
            BaseType_t unblocked;
            return esp_event_isr_post(event_base,
                                      event_id,
                                      event_data,
                                      event_data_size,
                                      &unblocked);
        }
#endif

        ESPEventAPICustom::ESPEventAPICustom(const esp_event_loop_args_t &event_loop_args)
        {
            esp_err_t res = esp_event_loop_create(&event_loop_args, &event_loop);
            if (res != ESP_OK)
            {
                throw idf::event::EventException(res);
            }
        }

        ESPEventAPICustom::~ESPEventAPICustom()
        {
            esp_event_loop_delete(event_loop);
        }

        esp_err_t ESPEventAPICustom::handler_register(esp_event_base_t event_base,
                                                      int32_t event_id,
                                                      esp_event_handler_t event_handler,
                                                      void *event_handler_arg,
                                                      esp_event_handler_instance_t *instance)
        {
            return esp_event_handler_instance_register_with(event_loop,
                                                            event_base,
                                                            event_id,
                                                            event_handler,
                                                            event_handler_arg,
                                                            instance);
        }

        esp_err_t ESPEventAPICustom::handler_unregister(esp_event_base_t event_base,
                                                        int32_t event_id,
                                                        esp_event_handler_instance_t instance)
        {
            return esp_event_handler_instance_unregister_with(event_loop, event_base, event_id, instance);
        }

        esp_err_t IRAM_ATTR ESPEventAPICustom::post(esp_event_base_t event_base,
                                                    int32_t event_id,
                                                    void *event_data,
                                                    size_t event_data_size,
                                                    TickType_t ticks_to_wait)
        {
            return esp_event_post_to(event_loop,
                                     event_base,
                                     event_id,
                                     event_data,
                                     event_data_size,
                                     ticks_to_wait);
        }

        esp_err_t ESPEventAPICustom::run(TickType_t ticks_to_run)
        {
            return esp_event_loop_run(event_loop, ticks_to_run);
        }

        //*---------------------------------------------------------freertos costum ----------------------------------
        //#ifdef CONFIG_HAL_MOCK

        ESPEventAPICustomFreeRTOS::ESPEventAPICustomFreeRTOS()
        {
            this->Start();
        }

        ESPEventAPICustomFreeRTOS::~ESPEventAPICustomFreeRTOS()
        {
            vTaskDelete(loopRunHandle);
            vQueueDelete(eventQueue);
        }
        esp_err_t ESPEventAPICustomFreeRTOS::Start()
        {
            eventQueue = xQueueCreate(10, sizeof(esp_event_post_instance_t));
            return xTaskCreate(LoopRunTaskWrap, "CustomloopTask", 4096, this, 10, &loopRunHandle) == pdPASS ? ESP_OK : ESP_FAIL;
        }

        esp_err_t ESPEventAPICustomFreeRTOS::post(esp_event_base_t event_base, int32_t event_id, void *event_data, size_t event_data_size, TickType_t ticks_to_wait)
        {
            esp_event_post_instance_t eventToPost = {event_base, event_id, event_data, event_data_size, NULL};
            return xQueueSend(eventQueue, &eventToPost, ticks_to_wait) == pdTRUE ? ESP_OK : ESP_FAIL;
        }

        esp_err_t ESPEventAPICustomFreeRTOS::handler_register(esp_event_base_t event_base, int32_t event_id, esp_event_handler_t event_handler, void *event_handler_arg, esp_event_handler_instance_t *instance)
        {
            esp_event_post_instance_t eventToRegister = {event_base, event_id, NULL, 0, event_handler_arg};
            handlersList[eventToRegister] = event_handler;
            return ESP_OK;
        }

        esp_err_t ESPEventAPICustomFreeRTOS::handler_unregister(esp_event_base_t event_base, int32_t event_id, esp_event_handler_instance_t instance)
        {
            esp_event_post_instance_t eventToUnRegister = {event_base, event_id, nullptr, 0, NULL};
            if (handlersList.find(eventToUnRegister) == handlersList.end())
            {
                return ESP_ERR_NOT_FOUND;
            }
            else
            {
                handlersList.erase(eventToUnRegister);
                return ESP_OK;
            }
        }

        void ESPEventAPICustomFreeRTOS::LoopRunTask()
        {
            while (1)
            {
                esp_event_post_instance_t event = {};
                xQueueReceive(eventQueue, &event, portMAX_DELAY);
                for (const auto &registeredList : handlersList)
                {
                    if (event == registeredList.first)
                    {
                        if (registeredList.second != nullptr)

                        {

                            // auto now = GET_NOW_MILLIS;
                            registeredList.second(registeredList.first.event_handler_arg, event.base, event.id, event.data);
                            //auto executionTime = GET_NOW_MILLIS - now;
                            //ESP_LOGD("EventLoop", " executing event %s %d, took %lldms", event.base, event.id, executionTime);
                        }
                    }
                }
            }
        }

        void ESPEventAPICustomFreeRTOS::LoopRunTaskWrap(void *arg)
        {
            if (arg != nullptr)
            {
                ESPEventAPICustomFreeRTOS *_this = (ESPEventAPICustomFreeRTOS *)arg;
                _this->LoopRunTask();
            }
            vTaskDelete(NULL);
        }
        //#endif
    } // event

} // idf

#endif // __cpp_exceptions
#endif