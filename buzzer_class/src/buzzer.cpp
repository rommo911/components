/*
 * buzzer.cpp
 *
 *  Created on: Jun 22, 2020
 *      Author: rami
 */
#include "debug_tools.h"

#if (DEBUG_BUZZER == VERBOS)
#define LOG_BUZZER LOGI
#define LOG_BUZZER_V LOGI
#elif (DEBUG_BUZZER == INFO)
#define LOG_BUZZER LOGI
#define LOG_BUZZER_V LOGD
#else
#define LOG_BUZZER LOGD
#define LOG_BUZZER_V LOGV
#endif
#include "assertion_tools.h"
#include "buzzer.h"
 //////////////////////////////////////////////////////////
 ///
 /// static variables, constants and configuration assignments
 ///
 /// /////////////////////////////////////////////////////

Buzzer::Buzzer(gpio_num_t _pin) : Task(TAG, CONFIG_BUZZER_TASK_STACK_SIZE, CONFIG_BUZZER_TASK_PRIORITY, CONFIG_BUZZER_TASK_CORE)
{
	maxFrequency = CONFIG_BUZZER_DEFAULT_FREQUENCY;
	maxPwm = CONFIG_BUZZER_ACK_DUTY;
	isBuzzing = false;
	isInitialized = false;
	this->ledcChannel.gpio_num = _pin;
}

/**
 * @fn esp_err_t Init(buzzerConfig_t&)
 * @brief	initialize led controller (PWM) by configuring a timer and installing
 * 			an led driver to be driven by this timer.
 *
 * @param config buzzerConfig_t the buzzer sequence to be installed
 * @return
 */
esp_err_t Buzzer::Init()
{
	LOG_BUZZER(TAG, "Init started");
	esp_err_t ret = ledc_timer_config(&ledcTimer);
	ret = ledc_channel_config(&ledcChannel);
	ret |= ledc_fade_func_install(0);
	ret |= ledc_set_duty_and_update(ledcChannel.speed_mode, ledcChannel.channel, 0, 0); //init at 0 out BUZ
	if (ret != ESP_OK && ret != ESP_ERR_NOT_FOUND)
	{
		ESP_LOGE("BUZZER_HAL", "ledc_fade_func_install failed with error : (%s)", esp_err_to_name(ret));
	}
	configuration = {};
	isInitialized = true;
	isBuzzing = false;
	//load default values to vectors of buzzing patteern
	//load values if configured to vectors of buzzing patteern
	LOG_BUZZER(TAG, "Init OK");
	return (ret);
}

/**
 * @brief save/modify buzzing sequence configuration // asserted
 *
 * @param config  buzzerConfig_t the Buzzer sequence to be installed
 * @return esp_err_t  ESP_FAIL : Buzzer is busy or nopt initialized
 */
esp_err_t Buzzer::SetConfig(const buzzerConfig_t& config)
{
	//assert
	if (isBuzzing || !isInitialized)
	{
		ESP_LOGE(TAG, "Buzzer is in use, or not initialzed");
		return ESP_FAIL;
	}
	else
	{
		configuration = config;
		LOG_BUZZER_V(TAG, "Buzzer configuration is set");
		return ESP_OK;
	}
}

/**
 * @brief start buzzing by creating a task.
 * 		buzzer configuration will be used is the one saved previousely
 * 		by SetConfigurationParameters()
 *
 *
 */
esp_err_t Buzzer::StartBuzzer()
{
	if (!isBuzzing && !TaskIsRunning() && isInitialized) // assert
	{
		StartTask();
	}
	else // assert failed
	{
		ESP_LOGW(TAG, "Buzzer start called but was already buzzing / or not initialized");
	}
	vTaskDelay(pdMS_TO_TICKS(10)); // wait for task to enter to assert
	if (isBuzzing)				   // this means task entered and buzzer is active
	{
		return ESP_OK;
	}
	else
	{
		ESP_LOGW(TAG, "Buzzer task problem..");
		return ESP_FAIL;
	}
}

/**
 * @brief stop the buzzer and delete its task if running
 *
 */
esp_err_t Buzzer::StopBuzzer()
{

	if (isBuzzing && this->TaskIsRunning())
	{
		configuration.isLoop = false;
		const uint16_t TIMEOUT = 5000;
		uint32_t _now = pdTICKS_TO_MS(xTaskGetTickCount());
		uint32_t xtimeout = _now + TIMEOUT;
		while (TaskIsRunning() && _now < xtimeout)
		{
			vTaskDelay(pdMS_TO_TICKS(200));
			_now = pdTICKS_TO_MS(xTaskGetTickCount());
		}

		if (TaskIsRunning()) // !failed to stop task within timeout : FORCE STOP
		{
			ESP_LOGE(TAG, "Buzzer task stopped responding, forcing task delete");
			StopTask(true); //!forceStop
		}
		//*just make sur buzzer is off
		ledc_timer_config_t _ledcTimer = ledcTimer;
		_ledcTimer.freq_hz = configuration.frequency[0];
		ledc_timer_config(&_ledcTimer);
		return ledc_set_duty_and_update(ledcChannel.speed_mode, ledcChannel.channel, 0, 0);
	}
	else
	{
		return ESP_ERR_INVALID_STATE;
	}
}

/**
 * @brief return buzzer status
 *
 * @return true  :buzzer is working
 * @return false
 */
bool Buzzer::IsBuzzing()
{
	return (isBuzzing);
}

/**
 * @brief buzzing task : will run in background
 * 		the task will loop forever if the Configuration.isLoop=true
 * 		to stop hte task call stop or set loop to false;
 *
 *
 * @param arg ignore.
 */
void Buzzer::run(void* arg)
{
	isBuzzing = true;
	do
	{
		for (auto i = 0; i < configuration.frequency.size(); i++)
		{
			ledc_timer_config_t _ledcTimer = ledcTimer;
			_ledcTimer.freq_hz = configuration.frequency[i];
			ledc_timer_config(&_ledcTimer);
			ledc_set_duty_and_update(ledcChannel.speed_mode, ledcChannel.channel, configuration.dutyCycle[i], 0);
			vTaskDelay(pdMS_TO_TICKS(configuration.delay[i]));
		}
	} while (configuration.isLoop);
	isBuzzing = false;
	ledc_timer_config_t _ledcTimer = ledcTimer;
	_ledcTimer.freq_hz = configuration.frequency[0];
	ledc_timer_config(&_ledcTimer);
	ledc_set_duty_and_update(ledcChannel.speed_mode, ledcChannel.channel, 0, 0);
}

/**
 * @brief for future purposes
 *
 * @param config
 * @return esp_err_t
 */
esp_err_t Buzzer::LaunchCustom(const buzzerConfig_t& config)
{
	size_t conf_sizes[3] = {
		config.delay.size(),
		config.dutyCycle.size(),
		config.frequency.size(),
	};
	if (!conf_sizes[0] || !conf_sizes[1] || !conf_sizes[2] ||
		conf_sizes[0] != conf_sizes[1] || conf_sizes[0] != conf_sizes[2] || conf_sizes[1] != conf_sizes[2])
	{
		return ESP_FAIL;
	}
	if (isBuzzing)
	{
		StopBuzzer();
	}
	SetConfig(config);
	return StartBuzzer();
}

Buzzer_t buzzer = nullptr;