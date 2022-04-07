/*
 * buzzer.h
 *
 *  Created on: Jun 22, 2020
 *      Author: rami
 */

#ifndef MAIN_BUZZER_H_
#define MAIN_BUZZER_H_
#pragma once

#include "system.hpp"
#include "task_tools.h"
#include <stdio.h>
#include <vector>
#include "driver/ledc.h"

class Buzzer : public Task
{
	public:

	struct buzzerConfig_t
	{
		std::vector<uint32_t> frequency; //Hz
		std::vector<uint16_t> delay;	 // ms
		std::vector<uint32_t> dutyCycle;
		bool isLoop;
	};
	private:
	bool isBuzzing;
	bool isInitialized;
	bool AlarmEnabled, ACkEnabled;
	buzzerConfig_t configuration;
	uint32_t maxFrequency, maxPwm;

	ledc_timer_config_t ledcTimer = {
		.speed_mode = LEDC_HIGH_SPEED_MODE,         // timer mode
		.duty_resolution = LEDC_TIMER_12_BIT,       // resolution of PWM duty
		.timer_num = LEDC_TIMER_0,                  // timer index
		.freq_hz = CONFIG_BUZZER_DEFAULT_FREQUENCY, // frequency of PWM signal
		.clk_cfg = LEDC_AUTO_CLK,                   // Auto select the source clock
	};
	ledc_channel_config_t ledcChannel = {
		.gpio_num = -1,  // gpio number (int)
		.speed_mode = LEDC_HIGH_SPEED_MODE, // timer mode
		.channel = LEDC_CHANNEL_0,
		.intr_type = LEDC_INTR_DISABLE,
		.timer_sel = LEDC_TIMER_0, // should be an active timer as the one above
		.duty = 0,                 // initial duty cycle
		.hpoint = 0,
		.flags = {0}
	};
	public:
	Buzzer(gpio_num_t BUZ_ENABLE_BUZZER_PIN);
	~Buzzer()
	{
		StopBuzzer();
	}

	//initialize pwm controller and pins for the buzzer (ledc)
	esp_err_t Init();
	//check if buzzer is active ( currently is working )
	bool IsBuzzing();
	//stop whatever is going on
	esp_err_t StopBuzzer();
	// start a custom buzzing operation
	// config : struct with proper buzzing sequence
	esp_err_t LaunchCustom(const buzzerConfig_t& config);
	// attach to mqtt commands
	esp_err_t ParseConfigCb(const char* topic, const char* data);
	//CONFIG OVERRIDE
	esp_err_t Diagnose();
	static constexpr char TAG[] = "buzzer";
	private:
	//freertos task : main buzzing loop
	void run(void* arg);
	//set config before calling start
	esp_err_t SetConfig(const buzzerConfig_t& config);
	//private : this will be called once config is set, it will lauch the  BuzzerTask
	esp_err_t StartBuzzer();
	//task is active and is buzzing
	void SetParams(uint32_t pwm, uint32_t frequency);
};

typedef std::unique_ptr<Buzzer> Buzzer_t;
extern Buzzer_t buzzer;

#endif /* MAIN_BUZZER_H_ */
