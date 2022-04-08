/*
 * FreeRTOS.cpp
 *
 *  Created on: Feb 24, 2017
 *      Author: kolban
 */
#include "FreeRTOS.hpp"
#include "debug_tools.h"
#include "esp_log.h"
#include "sdkconfig.h"
#include <iomanip>
#include <sstream>
static const char *TAG = "FreeRTOS";
#define DEBUG_SEM ESP_LOGI
//#define ENABLE_SEM_DEBUG

//static
std::map<const char *, TimerHandle_t, FreeRTOS::StrCompare> FreeRTOS::Timer::runningTimersList;

/**
 * Sleep for the specified number of milliseconds.
 * @param[in] ms The period in milliseconds for which to sleep.
 */
void FreeRTOS::sleep(uint32_t ms)
{
	::vTaskDelay(ms / portTICK_PERIOD_MS);
} // sleep

/**
 * Start a new task.
 * @param[in] task The function pointer to the function to be run in the task.
 * @param[in] taskName A string identifier for the task.
 * @param[in] param An optional parameter to be passed to the started task.
 * @param[in] stackSize An optional paremeter supplying the size of the stack in which to run the task.
 */
esp_err_t FreeRTOS::StartTask(void task(void*), const char* taskName, uint32_t stackSize, void* param , UBaseType_t uxPriority, TaskHandle_t* const pvCreatedTask , const BaseType_t xCoreID )
{
#ifdef ESP_PLATFORM
	if (xTaskCreatePinnedToCore(task, taskName, stackSize, param, uxPriority, pvCreatedTask,xCoreID) == pdPASS)
#else 
	if (xTaskCreate(task, taskName, stackSize, param, uxPriority, pvCreatedTask) == pdPASS)
#endif 
	{
		return ESP_OK; 
	}
	else
	{
		LOGE(TAG, " couldn't start task %s ", taskName);
		return ESP_FAIL;
	}
} // startTask

/**
 * Get the time in milliseconds since the %FreeRTOS scheduler started.
 * @return The time in milliseconds since the %FreeRTOS scheduler started.
 */
uint32_t FreeRTOS::millis()
{
	return (uint32_t)(xTaskGetTickCount() * portTICK_PERIOD_MS);
} // getTimeSinceStart

/**
 * @brief 
 * 
 * @param name Timer name 
 * @param millisSeconds time 
 * @param Periodic  true: repeated autoreload , else one shot 
 * @param arg 
 * @param cb  callback to be called 
 * @return TimerHandle_t 
 */
esp_err_t FreeRTOS::Timer::Create(const bool Periodic)
{
	//lock = Semaphore::CreateUnique(m_TimerName.c_str());
	m_TimerHandle = xTimerCreate(m_TimerName.c_str(), 10, (Periodic == true) ? pdTRUE : pdFALSE, this, TimerStaticCB);
	if (m_TimerHandle != nullptr)
	{
		runningTimersList[m_TimerName.c_str()] = m_TimerHandle;
		return ESP_OK;
	}
	else
	{
		ESP_LOGE(TAG, " couldn't creat timer %s ", m_TimerName.c_str());
		return ESP_FAIL;
	}

	return ESP_ERR_INVALID_ARG;
}

/**
 * @brief 
 * 
 * @param ms 
 * @param arg 
 * @return esp_err_t 
 */
esp_err_t FreeRTOS::Timer::TimerStart(std::chrono::milliseconds ms, void *arg)
{
	if (!TimerTaskIsRunning())
	{
		timerArgument = arg;
		m_TimerTick = pdMS_TO_TICKS(ms.count());
		TimerModify(m_TimerTick);
		AutoLock _lock(lock);
		BaseType_t ret = xTimerStart(m_TimerHandle, 0);
		return ret == pdPASS ? ESP_OK : ESP_FAIL;
	}
	return ESP_ERR_INVALID_STATE;
}

/**
 * @brief 
 * 
 * @return esp_err_t 
 */
esp_err_t FreeRTOS::Timer::TimerStop()
{
	if (!TimerTaskIsRunning())
	{
		AutoLock _lock(lock);
		BaseType_t ret = xTimerStop(m_TimerHandle, 0);
		return ret == pdPASS ? ESP_OK : ESP_FAIL;
	}
	return ESP_ERR_INVALID_STATE;
}

/**
 * @brief 
 * 
 * @param arg 
 */
void FreeRTOS::Timer::TimerStaticCB(TimerHandle_t arg)
{
	FreeRTOS::Timer *_this = static_cast<FreeRTOS::Timer *>(pvTimerGetTimerID(arg));
	AutoLock _lock(_this->lock);
	_this->TimerRun(_this->timerArgument);
}

/**
 * @brief 
 * 
 * @return true 
 * @return false 
 */
bool FreeRTOS::Timer::TimerTaskIsRunning() const
{
	return xTimerIsTimerActive(m_TimerHandle);
}

/**
 * @brief stop the timer and delete its handle 
 * 
 * @param handle 
 * @return esp_err_t 
 */
esp_err_t FreeRTOS::Timer::TimerDeinit()
{
	if (TimerTaskIsRunning())
	{
		if (TimerStop() != ESP_OK)
		{
			return ESP_FAIL;
		}
	}
	if (xTimerDelete(m_TimerHandle, 20) != pdPASS)
	{

		ESP_LOGE("FReeRTOS_Timer", "delete timer failed %s", m_TimerName.c_str());
		return ESP_FAIL;
	}
	m_TimerHandle = NULL;
	runningTimersList.erase(m_TimerName.c_str());
	return ESP_OK;
}

/**
 * @brief stop the timer, change its delay , and restart it 
 * 
 * @param hanlde 
 * @param millisSeconds 
 * @return esp_err_t 
 */
esp_err_t FreeRTOS::Timer::TimerModify(const uint32_t millisSeconds)
{
	AutoLock _lock(lock);
	return xTimerChangePeriod(m_TimerHandle, pdMS_TO_TICKS(millisSeconds), 100) == pdPASS ? ESP_OK : ESP_FAIL;
}

/**
 * @brief return freertos global number of task 
 * 
 * @return int 
 */
int FreeRTOS::GetRunningTaskNum()
{
	return uxTaskGetNumberOfTasks();
}

/**
 * @brief return freertos global list of task names 
 * 
 * @return std::string 
 */
std::string FreeRTOS::GetTaskList()
{
	char pcWriteBuffer[40 * 20]; // 40 char /task * 40 task ?
	vTaskList(pcWriteBuffer);
	return std::string(pcWriteBuffer);
}
