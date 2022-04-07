/*
 * Task.h
 *
 *  Created on: Mar 4, 2017
 *      Author: kolban
 */

#ifndef COMPONENTS_CPP_UTILS_TASK_H_
#define COMPONENTS_CPP_UTILS_TASK_H_
#pragma once
#include "FreeRTOS.hpp"
#include <list>
#include <memory>
#define CORE1 1
#define CORE0 0
/**
 * @brief Encapsulate a runnable task.
 *
 * This class is designed to be subclassed with the method:
 *
 * @code{.cpp}
 * void run(void *data) { ... }
 * @endcode
 */
class Task
{
private:
	static std::list<Task *> runningTasks;
	xTaskHandle m_handle = nullptr;
	void *m_taskData = nullptr;
	static void runTask(void *data);
	std::string m_taskName;
	uint16_t m_stackSize;
	uint8_t m_priority;
	BaseType_t m_coreId;
	TaskStatus_t m_pxTaskStatus = {};
	bool debug;
	static void DumpRunningTasks();

public:
	Task(const char *taskName = "Task", uint16_t stackSize = CONFIG_TASK_DEFAULT_STACK, uint8_t priority = CONFIG_TASK_DEFAULT_PRIORITY, BaseType_t core_id = tskNO_AFFINITY, bool _debug = false);
	~Task();
	bool TaskIsRunning() const;
	eTaskState GetTaskStatus();
	uint32_t GetTaskStackMin();
	static void Delay(int ms);
	static std::string GetRunningTasks();
	static void print_this_task_info();
	static std::string GetRunTimeStats();

protected:
	static SemaphorePointer_t GlobalTasLock;
	mutable SemaphorePointer_t ctlLock;
	mutable SemaphorePointer_t lock;
	void SetTaskStackSize(uint16_t stackSize);
	void SetTaskPriority(uint8_t priority);
	void SetTaskName(std::string name);
	void SetTaskCore(BaseType_t coreId);
	int GetTaskPriority() { return m_priority; };
	esp_err_t StartTask(void *taskData = nullptr);
	void StopTask(const bool force = false);
	void SuspendTask();
	void ResumeTask();
	void ActivateTaskDebug() { debug = true; };
	void DeActivateTaaskDebug() { debug = false; };
	const TaskStatus_t &GetTaskFullInfo();

	static const char *Status_ToString(eTaskState status);
	static unsigned long Millis();
	virtual void run(void *data) = 0; // Make run pure virtual

	// bool match_taskname(const std::string &value) { return (value == m_taskName); }
};

class AsyncTask
{
private:
	std::function<void(void *)> mainRun;
	const char *TAG;
	void *argument;
	TaskHandle_t handle;
	static std::mutex lock;

public:
	~AsyncTask();
	AsyncTask(std::function<void(void *)> cb, const char *_tag = "Async", void *arg = nullptr);
	static void StaticRun(void *arg);
};

#endif /* COMPONENTS_CPP_UTILS_TASK_H_ */
