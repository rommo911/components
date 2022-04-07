#include "pir.h"
#include "esp_log.h"
#include "string.h"

PIR::PIR(homeassistant::BaseDevCtx& hass_device, EventLoop_p_t& _loop, gpio_num_t pin, gpio_mode_t mode) : Task("PIR", 2048, 2, 1), TAG("PIR"),
loop(_loop), hassDevice(hass_device)
{
  setup(pin, mode);
}

PIR::~PIR()
{
}

void PIR::setup(gpio_num_t _pin, gpio_mode_t _mode)
{
  pin = _pin;
  events = xEventGroupCreate();
  gpio_config_t io_conf;
  memset(&io_conf, 0, sizeof(gpio_config_t));
  io_conf.mode = GPIO_MODE_INPUT;
  io_conf.intr_type = GPIO_INTR_ANYEDGE;
  io_conf.pin_bit_mask = (1ULL << pin);
  io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
  io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
  gpio_config(&io_conf);
  gpio_install_isr_service(0);
  gpio_isr_handler_add(pin, ISR_Handler, this);
  PIRLastStatus = false;
  PIRTimeout = 0;
  this->HassMotionDescovery = std::make_unique <homeassistant::BinarySensorDiscovery>(hassDevice, homeassistant::BinarySensorDiscovery::motion);
  this->HassPresenceDescovery = std::make_unique <homeassistant::BinarySensorDiscovery>(hassDevice, homeassistant::BinarySensorDiscovery::presence);
  StartTask(this);
}
void IRAM_ATTR PIR::ISR_Handler(void* arg)
{
  PIR* _this = (PIR*)(arg);
  xEventGroupSetBitsFromISR(_this->events, _this->ISR_EVENT, 0);
}

bool PIR::Status()
{
  return PIRLastStatus;
}

void PIR::run(void* arg)
{
  while (1)
  {
    auto bit = xEventGroupWaitBits(events, this->ISR_EVENT, pdTRUE, pdFALSE, pdMS_TO_TICKS(duration_cast<std::chrono::milliseconds>(timeOut).count()));
    auto _now = GET_NOW_SECONDS;
    if (bit & ISR_EVENT)
    {
      std::this_thread::sleep_for(20ms);
      status = gpio_get_level(pin);
      bool error = false;
      for (int i = 0; i < 3; i++)
      {
        if (status != gpio_get_level(pin))
        {
          error = true;
        }
        std::this_thread::sleep_for(20ms);
      }
      if (error)
        continue;
      if (status)
      {
        loop->post_event(EVENT_PIR_PIN_ON);
      }
      else
      {
        loop->post_event(EVENT_PIR_PIN_OFF);
      }
      if (status && status == PIRLastStatus)
      {

        PIRTimeout = _now + (timeOut.count());
        continue;
      }
    }
    else
    {
      status = gpio_get_level(pin);
    }
    if (PIRLastStatus != status)
    {
      //
      loop->post_event(EVENT_PIR_CHANGED);
      if (status)
      {
        PIRTimeout = _now + (timeOut.count());
        PIRLastStatus = status;
        ESP_LOGI("pir", "PIR on");
        loop->post_event(EVENT_PIR_ON);
      }
      else
      {
        if (_now > PIRTimeout)
        {
          ESP_LOGI("pir", "PIR off");
          PIRLastStatus = status;
          loop->post_event(EVENT_PIR_OFF);
          continue;
        }
      }
    }
  }
}

PIR* pir = nullptr;

SWITCH::SWITCH(homeassistant::BaseDevCtx& hass_device,const char * type, EventLoop_p_t& _loop, gpio_num_t _pin, gpio_mode_t _mode) : Task("PIR"), loop(_loop), hassDevice(hass_device)
{
  Setup(_pin, _mode,type);
}

void SWITCH::Setup(gpio_num_t _pin, gpio_mode_t _mode, const char * type)
{
  pin = _pin;
  SWITCH_events = xEventGroupCreate();
  gpio_config_t io_conf;
  memset(&io_conf, 0, sizeof(gpio_config_t));
  io_conf.mode = _mode;
  io_conf.intr_type = GPIO_INTR_ANYEDGE;
  io_conf.pin_bit_mask = (1ULL << pin);
  io_conf.pull_down_en = GPIO_PULLDOWN_ENABLE;
  io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
  gpio_config(&io_conf);
  gpio_install_isr_service(0);
  gpio_isr_handler_add(pin, ISR_Handler, this);
  status = gpio_get_level(pin);
  this->HassSwitchDescovery = std::make_unique<homeassistant::BinarySensorDiscovery>(hassDevice, type);
  StartTask(this);

}


void IRAM_ATTR SWITCH::ISR_Handler(void* arg)
{
  SWITCH* _this = static_cast<SWITCH*>(arg);
  xEventGroupSetBitsFromISR(_this->SWITCH_events, _this->ISR_EVENT, 0);
  _this->status = gpio_get_level(_this->pin);
}

bool SWITCH::Status()
{
  return status;
}

void SWITCH::run(void* arg)
{
  while (1)
  {
    xEventGroupWaitBits(this->SWITCH_events, this->ISR_EVENT, pdTRUE, pdTRUE, portMAX_DELAY);
    if (status != lastStatus)
    {
      xEventGroupSetBits(SWITCH_events, Changed_EVENT);
      loop->post_event(EVENT_SW_CHANGED);
      if (status)
      {
        ESP_LOGI("SW", "SW on");
        loop->post_event(EVENT_SW_ON);
      }
      else
      {
        ESP_LOGI("SW", "SW off");
        loop->post_event(EVENT_SW_OFF);
      }
      lastStatus = status;
    }
    vTaskDelay(pdMS_TO_TICKS(500));
  }
}

SWITCH* Switch;