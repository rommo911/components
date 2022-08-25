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
std::mutex Task::GlobalTasLock;
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
} // Task

Task::~Task()
{
	StopTask(true);
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
	if (pTask->mainRunFunction != nullptr)
	{
		pTask->mainRunFunction(pTask->m_taskData);
	}
	else
	{
		pTask->run(pTask->m_taskData);
	}
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
	std::unique_lock __Lock(taskcCtlLock, std::chrono::seconds(2));
	if (!__Lock.owns_lock())
	{
		return ESP_FAIL;
	}
#ifdef ESP_PLATFORM
	if (xTaskCreatePinnedToCore(&runTask, m_taskName.c_str(), m_stackSize, this, m_priority, &m_handle, m_coreId) == pdPASS)
#else
	if (xTaskCreate(&runTask, m_taskName.c_str(), m_stackSize, this, m_priority, &m_handle) == pdPASS)
#endif
	{
		runningTasks.push_back(this);
		return ESP_OK;
	}
	else
	{
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
	runningTasks.remove_if([&](const Task *tsk)
						   { return tsk == this; });
	if (!force)
	{
	if (!taskcCtlLock.try_lock_for(std::chrono::seconds(2)))
		{
			return ;
		}
	}
	TaskHandle_t tempHandle = this->m_handle;
	this->m_handle = nullptr;
	vTaskDelete(tempHandle);
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

const TaskStatus_t &Task::GetTaskFullInfo()
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

esp_err_t Task::SetRunTask(std::function<void(void *)> fn)
{
	if (this->TaskIsRunning())
	{
		return ESP_ERR_INVALID_STATE;
	}
	if (fn == nullptr)
	{
		return ESP_ERR_INVALID_ARG;
	}
	this->mainRunFunction = fn;
	return ESP_OK;
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
			std::unique_lock __Lock(taskcCtlLock);
			vTaskSuspend(m_handle);
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
			std::unique_lock __Lock(taskcCtlLock);
			vTaskResume(m_handle);
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
	return xTaskGetTickCount();
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
	for (const auto &task : runningTasks)
	{
		std::stringstream ss;
		ss << "task name:" << task->m_taskName.c_str() << ", status:" << Task::Status_ToString(task->GetTaskStatus()) << ", core:" << task->m_coreId << ", prio: " << (task->m_priority) << ", minimum free stack:" << uxTaskGetStackHighWaterMark(task->m_handle) << " bytes";
		ESP_LOGI("TASK INFO", "%s", ss.str().c_str());
	}
}

std::string Task::GetRunningTasks()
{

	std::stringstream ss;
	for (const auto &task : runningTasks)
	{
		ss << "task name:" << task->m_taskName.c_str() << ", status:" << Task::Status_ToString(task->GetTaskStatus()) << ", core:" << task->m_coreId << ", prio: " << (task->m_priority) << ", minimum free stack:" << uxTaskGetStackHighWaterMark(task->m_handle) << " bytes"
		   << "\n ";
	}
	return ss.str();
}

std::string Task::GetRunTimeStats()
{
	std::string ret("N/A");
#if (configGENERATE_RUN_TIME_STATS == 1)
	char *buffer = new char[40 * 20];
	vTaskGetRunTimeStats(buffer);
	ret = std::string(buffer);
	delete (buffer);
#endif
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
	ss << "task name:" << pcTaskGetTaskName(nullptr) << ",Core id:" << xPortGetCoreID() << ",prio:" << uxTaskPriorityGet(nullptr) << ",minimum free stack:" << uxTaskGetStackHighWaterMark(nullptr) << "bytes";
	ESP_LOGI("TASK INFO", "%s\n\r", ss.str().c_str());
}

////////////////////////////////////////

std::mutex AsyncTask::lock;

AsyncTask::~AsyncTask()
{
	if (handle != nullptr)
	{
		vTaskDelete(handle);
	}
}

AsyncTask::AsyncTask(std::function<void(void *)> cb, const char *_tag, void *arg) : mainRun(cb), TAG(_tag), argument(arg)
{
	std::lock_guard L(AsyncTask::lock);
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

void AsyncTask::StaticRun(void *arg)
{
	AsyncTask *_this = static_cast<AsyncTask *>(arg);
	if (_this != nullptr)
	{
		_this->mainRun(_this->argument);
	}
	TaskHandle_t this_handle = _this->handle;
	_this->handle = nullptr;
	vTaskDelete(this_handle);
}
