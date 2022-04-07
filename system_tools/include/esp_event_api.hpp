#ifndef ESP_EVENT_API_HPP_
#define ESP_EVENT_API_HPP_
#include "sdkconfig.h"
#ifdef ESP_PLATFORM
#include "esp_event.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#else
#include "FreeRTOS.h" // for tickType_t
#include "esp_err.h"
typedef void (*esp_event_handler_t)(esp_event_base_t event_base, int32_t event_id, void *event_data, size_t event_data_len);
typedef void *esp_event_handler_instance_t; /**< context identifying an instance of a registered event handler */
typedef const char *esp_event_base_t;
#endif
#include "string.h"
#include <cstring>
#include <iostream>
#include <map>
namespace idf
{

        namespace event
        {

                /**
                 * Abstract interface for direct calls to esp_event C-API.
                 * This is generally not intended to be used directly.
                 * It's main purpose is to provide ESPEventLoop a unified API not dependent on whether the default event loop or a
                 * custom event loop is used.
                 * The interface resembles the C-API, have a look there for further documentation.
                 */
                class ESPEventAPI
                {
                public:
                        virtual ~ESPEventAPI() {}

                        virtual esp_err_t handler_register(esp_event_base_t, // eventbase
                                                           int32_t,          // evetn id
                                                           esp_event_handler_t,
                                                           void *, // arg
                                                           esp_event_handler_instance_t *) = 0;

                        virtual esp_err_t handler_unregister(esp_event_base_t,
                                                             int32_t,
                                                             esp_event_handler_instance_t) = 0;

                        virtual esp_err_t post(esp_event_base_t,
                                               int32_t,
                                               void *,
                                               size_t,
                                               TickType_t) = 0;
#if CONFIG_ESP_EVENT_POST_FROM_ISR
                        virtual esp_err_t post_from_isr(esp_event_base_t event_base,
                                                        int32_t event_id,
                                                        void *event_data,
                                                        size_t event_data_size)
                        {
                                return ESP_FAIL;
                        };
#endif // CONFIG_ESP_EVENT_POST_FROM_ISR
                };

                /**
                 * @brief API version with default event loop.
                 *
                 * It will direct calls to the default event loop API.
                 */
#ifdef ESP_PLATFORM

                class ESPEventAPIDefault : public ESPEventAPI
                {
                public:
                        ESPEventAPIDefault();
                        virtual ~ESPEventAPIDefault();

                        /**
                         * Copying would lead to deletion of event loop through destructor.
                         */
                        ESPEventAPIDefault(const ESPEventAPIDefault &o) = delete;
                        ESPEventAPIDefault &operator=(const ESPEventAPIDefault &) = delete;

                        esp_err_t handler_register(esp_event_base_t event_base,
                                                   int32_t event_id,
                                                   esp_event_handler_t event_handler,
                                                   void *event_handler_arg,
                                                   esp_event_handler_instance_t *instance) override;

                        esp_err_t handler_unregister(esp_event_base_t event_base,
                                                     int32_t event_id,
                                                     esp_event_handler_instance_t instance) override;

                        esp_err_t post(esp_event_base_t event_base,
                                       int32_t event_id,
                                       void *event_data,
                                       size_t event_data_size,
                                       TickType_t ticks_to_wait) override;
#if CONFIG_ESP_EVENT_POST_FROM_ISR

                        esp_err_t post_from_isr(esp_event_base_t event_base,
                                                int32_t event_id,
                                                void *event_data,
                                                size_t event_data_size) override;
#endif // CONFIG_ESP_EVENT_POST_FROM_ISR
                };

#else // ESP_PLATFORM
                class ESPEventAPIDefault : public ESPEventAPI
                {
                public:
                        ESPEventAPIDefault();
                        virtual ~ESPEventAPIDefault();

                        /**
                         * Copying would lead to deletion of event loop through destructor.
                         */
                        ESPEventAPIDefault(const ESPEventAPIDefault &o) = delete;
                        ESPEventAPIDefault &operator=(const ESPEventAPIDefault &) = delete;

                        esp_err_t handler_register(esp_event_base_t event_base,
                                                   int32_t event_id,
                                                   esp_event_handler_t event_handler,
                                                   void *event_handler_arg,
                                                   esp_event_handler_instance_t *instance) override
                        {
                                return ESP_OK;
                        }

                        esp_err_t handler_unregister(esp_event_base_t event_base,
                                                     int32_t event_id,
                                                     esp_event_handler_instance_t instance) override
                        {
                                return ESP_OK;
                        }

                        esp_err_t post(esp_event_base_t event_base,
                                       int32_t event_id,
                                       void *event_data,
                                       size_t event_data_size,
                                       TickType_t ticks_to_wait) override
                        {
                                std::cout << "event post :" << event_base << "id= " << event_id;
                                return ESP_OK;
                        }
#if CONFIG_ESP_EVENT_POST_FROM_ISR

                        esp_err_t post_from_isr(esp_event_base_t event_base,
                                                int32_t event_id,
                                                void *event_data,
                                                size_t event_data_size) override
                        {
                                return ESP_OK;
                        }
#endif // CONFIG_ESP_EVENT_POST_FROM_ISR
                };

#endif // CONFIG_HAL_MOCK

                /**
                 * @brief API version with custom event loop.
                 *
                 * It will direct calls to the custom event loop API.
                 * The loop parameters are given in the constructor the same way it's done in esp_event_loop_create() in event.h.
                 * This class also provides a run method in case the custom event loop was created without its own task.
                 */
#ifdef ESP_PLATFORM

                class ESPEventAPICustom : public ESPEventAPI
                {
                public:
                        /**/
                        ESPEventAPICustom(const esp_event_loop_args_t &event_loop_args);
                        virtual ~ESPEventAPICustom();
                        /***/
                        ESPEventAPICustom(const ESPEventAPICustom &o) = delete;
                        ESPEventAPICustom &operator=(const ESPEventAPICustom &) = delete;

                        esp_err_t handler_register(esp_event_base_t event_base,
                                                   int32_t event_id,
                                                   esp_event_handler_t event_handler,
                                                   void *event_handler_arg,
                                                   esp_event_handler_instance_t *instance) override;

                        esp_err_t handler_unregister(esp_event_base_t event_base,
                                                     int32_t event_id,
                                                     esp_event_handler_instance_t instance) override;

                        esp_err_t post(esp_event_base_t event_base,
                                       int32_t event_id,
                                       void *event_data,
                                       size_t event_data_size,
                                       TickType_t ticks_to_wait) override;

                        esp_err_t run(TickType_t ticks_to_run);
                        const esp_event_loop_handle_t &Handle() { return event_loop; };

                private:
                        esp_event_loop_handle_t event_loop;
                };

#endif //  ESP_PLATFORM //class ESPEventAPICustom
                class ESPEventAPICustomFreeRTOS : public ESPEventAPI
                {
                public:
                        ESPEventAPICustomFreeRTOS();
                        virtual ~ESPEventAPICustomFreeRTOS();
                        ESPEventAPICustomFreeRTOS(const ESPEventAPICustomFreeRTOS &o) = delete;
                        ESPEventAPICustomFreeRTOS &operator=(const ESPEventAPICustomFreeRTOS &) = delete;
                        esp_err_t handler_register(esp_event_base_t event_base, int32_t event_id, esp_event_handler_t event_handler, void *event_handler_arg, esp_event_handler_instance_t *instance) override;
                        esp_err_t handler_unregister(esp_event_base_t event_base, int32_t event_id, esp_event_handler_instance_t instance) override;
                        esp_err_t post(esp_event_base_t event_base, int32_t event_id, void *event_data, size_t event_data_size, TickType_t ticks_to_wait) override;
                        esp_err_t Start();
                        static void LoopRunTaskWrap(void *arg);
                        void LoopRunTask();

                private:
                        TaskHandle_t loopRunHandle = nullptr;
                        QueueHandle_t eventQueue = nullptr;
                        struct esp_event_post_instance_t
                        {
                                esp_event_base_t base; /**< the event base */
                                int32_t id;            /**< the event id */
                                void *data;            /**< data associated with the event */
                                size_t dataSize;       /**< data associated with the event */
                                void *event_handler_arg;
                                bool operator==(const esp_event_post_instance_t &event)
                                {
                                        return (id == event.id) && (strcmp(base, event.base) == 0);
                                }
                        };
                        struct eventCompare
                        {
                        public:
                                bool operator()(const esp_event_post_instance_t f, const esp_event_post_instance_t s) const
                                {
                                        return std::strcmp(f.base, s.base) < 0;
                                }
                        };
                        std::map<esp_event_post_instance_t, esp_event_handler_t, eventCompare> handlersList;
                };
        } // event

} // idf

#endif // ESP_EVENT_API_HPP_
