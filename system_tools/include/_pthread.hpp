#ifndef __ALADIN_PTHREAD_H__
#define __ALADIN_PTHREAD_H__

#include <thread>
#include <mutex>
#include <functional>
#include <queue>
#include <condition_variable>

#ifdef ESP_PLATFORM
#include <esp_pthread.h>
#include "esp_log.h"
#endif

class AladinPthread_t
{
private:
  std::thread _thread;
  static std::mutex _mutex;
#ifdef ESP_PLATFORM
  static esp_pthread_cfg_t CreateConfig(const char *name, int core_id, int stack, int prio);
#endif

  bool started = false;

public:
  AladinPthread_t() = delete;

  template <typename _Callable, typename... _Args>
  AladinPthread_t(const char *name, int core_id, int stack, int prio, _Callable fn, _Args &&...__args);

  template <typename _Callable, typename... _Args>
  AladinPthread_t(_Callable fn, _Args &&...__args);

  ~AladinPthread_t()
  {
    if (_thread.joinable())
      _thread.detach();
  }

  void Join();
  bool Joinable() { return this->_thread.joinable(); }
  static uint32_t GetStackWaterMark();
};

template <typename _Callable, typename... _Args>
AladinPthread_t::AladinPthread_t(const char *name, int core_id, int stack, int prio, _Callable fn, _Args &&...__args)
{
  _mutex.lock();
#ifdef ESP_PLATFORM
  esp_pthread_cfg_t config = CreateConfig(name, core_id, stack, prio);
  esp_pthread_set_cfg(&config);
#endif
  this->_thread = std::thread(std::forward<_Callable>(fn), std::forward<_Args>(__args)...);
  _mutex.unlock();
  started = true;
}

template <typename _Callable, typename... _Args>
AladinPthread_t::AladinPthread_t(_Callable fn, _Args &&...__args)
{
  _mutex.lock();
#ifdef ESP_PLATFORM
  esp_pthread_cfg_t config = esp_pthread_get_default_config();
  esp_pthread_set_cfg(&config);
#endif
  this->_thread = std::thread(std::forward<_Callable>(fn), std::forward<_Args>(__args)...);
  _mutex.unlock();
  started = true;
}

#ifndef SAFE_QUEUE
#define SAFE_QUEUE

// A threadsafe-queue.
template <typename T>
class SafeQueue
{
public:
  SafeQueue(void)
      : queue(), mutex(), cond()
  {
  }

  ~SafeQueue(void)
  {
  }

  // Add an element to the queue.
  void enqueue(T value)
  {
    std::unique_lock lock(mutex);
    queue.push(value);
    cond.notify_one();
  }

  // Get the "front"-element.
  // If the queue is empty, wait till a element is avaiable.
  T dequeue(void)
  {
    std::unique_lock lock(mutex);
    while (queue.empty())
    {
      // release lock as long as the wait and reaquire it afterwards.
      cond.wait(lock);
    }
    T val = queue.front();
    queue.pop();
    return val;
  }
  // Get the "front"-element.
  // If the queue is empty, wait till a element is avaiable.

  std::optional<T> dequeue_wait_for(std::chrono::milliseconds ms)
  {
    std::unique_lock lock(mutex);
    bool res = cond.wait_for(lock, ms) != std::cv_status::timeout;
    if (queue.empty() || res)
    {
      //ESP_LOGE("queue", "empty or timed out = %d", res);
      return std::nullopt;
    }
    T val = queue.front();
    queue.pop();
    return val;
  }
  std::optional<T> get_front()
  {
    if (queue.empty())
    {
      return std::nullopt;
    }
    T val = queue.front();
    return val;
  }

private:
  std::queue<T> queue;
  mutable std::mutex mutex;
  std::condition_variable cond;
};
#endif

#endif // __ALADIN_PTHREAD_H__