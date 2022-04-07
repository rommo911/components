#ifndef __SEMPHR_H__
#define __SEMPHR_H__
#pragma once

#include "sdkconfig.h"
#include "string.h"
#include <chrono>
#include <cstring>
#ifdef ESP_PLATFORM
#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>
#include <freertos/semphr.h> // Include the semaphore definitions.
#include <freertos/task.h>
#include <freertos/timers.h>
#else
#include <FreeRTOS.h>
#include <event_groups.h>
#include <semphr.h> // Include the semaphore definitions.
#include <task.h>
#include <timers.h>
#endif
#include <memory>
#include <mutex>
class Semaphore
{
public:
    Semaphore(const char *owner = "<N/A>", uint8_t n = 0);
    ~Semaphore();
    static std::unique_ptr<Semaphore> CreateUnique(const char *);
    //Semaphore give back ( asserted )
    bool unlock();
    //Semaphore give back from interrupt ( not asserted )
    void giveFromISR();
    //set the semaphore name
    void setName(std::string name);
    //Semaphore take wait forever
    bool lock(const char *owner);
    bool lock();
    //Semaphore take wait for timeout in Ms
    bool lock(std::chrono::milliseconds, const char *owner = "<N/A>");
    //return current semaphore name and current owner
    std::string toString();
    // take the semaphore ( wait for timout in ms) and give it back immidiately
    bool wait(const char *owner, std::chrono::milliseconds timeout = std::chrono::milliseconds(5000));
    //activate the give/take logging
protected:
    void ActivateDebug();
    //Deactivate the give/take logging
    void DeActivateDebug();

private:
    SemaphoreHandle_t m_semaphore;
    std::string m_name;
    std::string m_owner;
    uint32_t m_value;
    bool m_usePthreads;
    bool taken;
    bool debug = false;
};

class SemaphoreN : public Semaphore
{ // wrapper for counting semaphore
private:
    /* data */
public:
    SemaphoreN(const char *owner, uint8_t n) : Semaphore(owner, n) {}
    static std::unique_ptr<SemaphoreN> CreateUniqueM(const char *tag, uint8_t n);
    ~SemaphoreN() {}
};

using SemaphorePointer_t = std::unique_ptr<Semaphore>;
using SemaphoreNPointer_t = std::unique_ptr<SemaphoreN>;

class AutoLock
{
private:
    SemaphorePointer_t &m_sem;
    bool locked = false;

public:
    explicit AutoLock(SemaphorePointer_t &sem, uint8_t timeoutSecods = 2, const char *txt = "n/a") : m_sem(sem)
    {
        if (sem == nullptr)
        {
            //ESP_LOGE("semaphore", "%s is null", txt);
            return;
        }
        if (!m_sem->lock(std::chrono::seconds(timeoutSecods), txt))
        {
            //ESP_LOGE("autoLock", "Failed %s", txt);
        }
        locked = true;
    }
    ~AutoLock()
    {
        if (locked)
            m_sem->unlock();
    }
};

class AutoLockMutex
{
public:
    explicit AutoLockMutex(std::timed_mutex &mut,
                           const char *txt = "n/a",
                           std::chrono::milliseconds ms = std::chrono::milliseconds(10000)) : m_metx(mut)
    {
        m_metx.try_lock_for(ms);
    }
    ~AutoLockMutex()
    {
        m_metx.unlock();
    }

private:
    std::timed_mutex &m_metx;
};

#endif // __SEMPHR_H__