/*
 * Task.cpp
 *
 *  Created on: Mar 4, 2017
 *      Author: kolban
 */
#include "sdkconfig.h"
#include "esp_err.h"
#include "task_tools.h"
#include "debug_tools.h"
#include "sdkconfig.h"
#include <esp_log.h>
#ifdef ESP_PLATFORM
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#else
#include <FreeRTOS.h>
#include <task.h>
#endif
#include <sstream>
#include <string>

static const char *LOG_TAG = "Task";
std::list<Task *> Task::runningTasks = {};
SemaphorePointer_t Task::GlobalTasLock = nullptr;
/**
 * @brief Create an instance of the task class.
 *
 * @param [in] taskName The name of the task to create.
 * @param [in] stackSize The size of the stack.
 * @return N/A.
 */
Task::Task(const char *taskName, uint16_t stackSize, uint8_t priority, BaseType_t core_id, bool _debug) : m_taskName(taskName),
																										  m_stackSize(stackSize),
																										  m_priority(priority),
																										  m_coreId(core_id),
																										  debug(_debug)
{
	std::string sem = m_taskName + "ctl";
	ctlLock = Semaphore::CreateUnique(sem.c_str());
	lock = Semaphore::CreateUnique(taskName);
	/*	if (GlobalTasLock == nullptr)//TODO
	{
		GlobalTasLock = Semaphore::CreateUnique("Global");
	}*/

} // Task

Task::~Task()
{
	StopTask();

} // ~Task

/**
 * @brief Suspend the task for the specified milliseconds.
 *
 * @param [in] ms The delay time in milliseconds.
 * @return N/A.
 */

/* static */ void Task::Delay(int ms)
{
	::vTaskDelay(pdMS_TO_TICKS(ms));
} // delay

/**
 * Static class member that actually runs the target task.
 *
 * The code here will run on the task thread.
 * @param [in] pTaskInstance The task to run.
 */
void Task::runTask(void *pTaskInstance)
{
	Task *pTask = (Task *)pTaskInstance;
	if (pTask->debug)
		ESP_LOGI(LOG_TAG, ">> Task entering : taskName=%s", pTask->m_taskName.c_str());
	pTask->run(pTask->m_taskData);
	if (pTask->debug)
		ESP_LOGI(LOG_TAG, "<< Task exiting: taskName=%s", pTask->m_taskName.c_str());
	pTask->StopTask();
} // runTask

/**
 * @brief Start an instance of the task.
 *
 * @param [in] taskData Data to be passed into the task.
 * @return N/A.
 */
esp_err_t Task::StartTask(void *taskData)
{
	if (m_handle != nullptr)
	{
		LOGE(LOG_TAG, "Task::start - There might be a task already running!");
		return ESP_ERR_INVALID_STATE;
	}
	m_taskData = taskData;
	ctlLock->lock("start_fn");
#ifdef ESP_PLATFORM
	if (xTaskCreatePinnedToCore(&runTask, m_taskName.c_str(), m_stackSize, this, m_priority, &m_handle, m_coreId) == pdPASS)
#else
	if (xTaskCreate(&runTask, m_taskName.c_str(), m_stackSize, this, m_priority, &m_handle) == pdPASS)
#endif
	{
		runningTasks.push_back(this);
		ctlLock->unlock();
		return ESP_OK;
	}
	else
	{
		ctlLock->unlock();
		return ESP_ERR_NO_MEM;
	}
} // start

/**
 * @brief Stop the task.
 *
 * @return N/A.
 */
void Task::StopTask(const bool force)
{
	if (m_handle == nullptr)
		return;
	auto buf = m_taskName;
	auto fn2 = [&](const Task *tsk)
	{ return tsk == this; };
	runningTasks.remove_if(fn2);
	if (!force)
		ctlLock->wait("StopTask");
	xTaskHandle temp = m_handle;
	m_handle = nullptr;
	::vTaskDelete(temp);

} // stop

/**
 * @brief Set the stack size of the task.
 *
 * @param [in] stackSize The size of the stack for the task.
 * @return N/A.
 */
void Task::SetTaskStackSize(uint16_t stackSize)
{
	m_stackSize = stackSize;
}

const TaskStatus_t IRAM_ATTR &Task::GetTaskFullInfo()
{
	vTaskGetInfo(this->m_handle, &m_pxTaskStatus, pdTRUE, eBlocked);
	return m_pxTaskStatus;
}

/**
 * @brief Set the priority of the task.
 *
 * @param [in] priority The priority for the task.
 * @return N/A.
 */
void IRAM_ATTR Task::SetTaskPriority(uint8_t priority)
{
	if (TaskIsRunning())
	{
		vTaskPrioritySet(m_handle, priority);
	}
	else
	{
		m_priority = priority;
	}
}

/**
 * @brief Set the name of the task.
 *
 * @param [in] name The name for the task.
 * @return N/A.
 */
void Task::SetTaskName(std::string name)
{
	m_taskName = name;
} // setName

/**
 * @brief Set the core number the task has to be executed on.
 * If the core number is not set, tskNO_AFFINITY will be used
 *
 * @param [in] coreId The id of the core.
 * @return N/A.
 */
void Task::SetTaskCore(BaseType_t coreId)
{
	m_coreId = coreId;
}

/**
 * @brief
 *
 */
void Task::SuspendTask()
{
	if (m_handle != nullptr)
	{
		if (eTaskGetState(m_handle) != eSuspended)
		{
			ctlLock->lock("SuspendTask");
			vTaskSuspend(m_handle);
			ctlLock->unlock();
		}
	}
}

/**
 * @brief
 *
 */
void Task::ResumeTask()
{
	if (m_handle != nullptr)
	{
		if (eTaskGetState(m_handle) == eSuspended)
		{
			ctlLock->lock("ResumeTask");
			vTaskResume(m_handle);
			ctlLock->unlock();
		}
	}
}

/**
 * @brief
 *
 * @return true
 * @return false
 */
bool Task::TaskIsRunning() const
{
	if (m_handle != nullptr)
	{
		eTaskState status = eTaskGetState(m_handle);
		if (status == eBlocked || status == eReady || status == eRunning)
		{
			return true;
		}
	}
	return false;
}

/**
 * @brief
 *
 * @return unsigned long
 */
unsigned long IRAM_ATTR Task::Millis()
{
	return pdTICKS_TO_MS(xTaskGetTickCount());
}

/**
 * @brief
 *
 * @return eTaskState
 */
eTaskState IRAM_ATTR Task::GetTaskStatus()
{
	return eTaskGetState(m_handle);
}

uint32_t IRAM_ATTR Task::GetTaskStackMin()
{
	return uxTaskGetStackHighWaterMark(m_handle);
}

/**
 * @brief
 *
 */
void Task::DumpRunningTasks()
{
	for (auto task : runningTasks)
	{
		std::stringstream ss;
		ss << "task name:" << task->m_taskName.c_str() << ", status:" << Task::Status_ToString(task->GetTaskStatus()) << ", core:" << task->m_coreId << ", prio: " << (task->m_priority) << ", minimum free stack:" << uxTaskGetStackHighWaterMark(task->m_handle) << " bytes";
		ESP_LOGI("TASK INFO", "%s", ss.str().c_str());
	}
}

std::string Task::GetRunningTasks()
{

	std::stringstream ss;
	for (auto task : runningTasks)
	{

		ss << "task name:" << task->m_taskName.c_str() << ", status:" << Task::Status_ToString(task->GetTaskStatus()) << ", core:" << task->m_coreId << ", prio: " << (task->m_priority) << ", minimum free stack:" << uxTaskGetStackHighWaterMark(task->m_handle) << " bytes"
		   << "\n ";
	}
	return ss.str();
}



std::string Task::GetRunTimeStats()
{
	char *buffer = new char[40 * 20];
	vTaskGetRunTimeStats(buffer);
	std::string ret(buffer);
	delete (buffer);
	return ret;
}

/**
 * @brief
 *
 * @param status
 * @return const char*
 */
const char *Task::Status_ToString(eTaskState status)
{
	switch (status)
	{
	case eRunning:
		return "Running";
		break;
	case eReady:
		return "Ready";
		break;
	case eBlocked:
		return "Blocked";
		break;
	case eSuspended:
		return "Suspended";
		break;
	case eDeleted:
		return "Deleted";
		break;
	default:
		return "N/A";
		break;
	}
}

void Task::print_this_task_info()
{
#ifdef CONFIG_HAL_MOCK
#define xPortGetCoreID() "no_core"
#endif
	std::stringstream ss;
	ss << "task name:" << pcTaskGetTaskName(nullptr) << ", Core id:" << xPortGetCoreID() << ", prio: " << uxTaskPriorityGet(nullptr) << ", minimum free stack:" << uxTaskGetStackHighWaterMark(nullptr) << " bytes";
	ESP_LOGI("TASK INFO", "%s", ss.str().c_str());
}

AsyncTask::~AsyncTask()
{
	// LOGI(TAG, " AsyncTask destroying %d ", (int)this);
	vTaskDelete(handle);
}

AsyncTask::AsyncTask(std::function<void(void *)> cb, const char *_tag, void *arg) : mainRun(cb), TAG(_tag), argument(arg)
{
#ifdef ESP_PLATFORM
	if (xTaskCreatePinnedToCore(StaticRun, "fastTask", 4092, this, 1, &handle, 0) == pdPASS)
#else
	if (xTaskCreate(StaticRun, "fastTask", 4092, this, 1, &handle) == pdPASS)
#endif
	{
		// LOGI(TAG, " AsyncTask entered %d ", (int)this);
		return;
	}
	else
	{
		LOGE(TAG, " couldn't start task ");
		return;
	}
}
std::mutex AsyncTask::lock;

void AsyncTask::StaticRun(void *arg)
{
	const AsyncTask *_this = static_cast<AsyncTask *>(arg);
	if (_this != nullptr)
	{
		AsyncTask::lock.lock();
		_this->mainRun(_this->argument);
		AsyncTask::lock.unlock();
	}
	delete (_this);
	_this = nullptr;
}
