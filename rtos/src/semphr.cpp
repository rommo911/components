#include "semphr.hpp"

/**
 * @brief Wait for a semaphore to be released by trying to take it and
 * then releasing it again.
 * @param [in] owner A debug tag.
 * @return The value associated with the semaphore.
 */
bool Semaphore::wait(const char *owner, std::chrono::milliseconds timout)
{
#ifdef ENABLE_SEM_DEBUG
    if (debug)
        DEBUG_SEM(TAG, ">> wait: Semaphore waiting: %s for %s", toString().c_str(), owner);
    m_owner = owner;
#endif
    TickType_t ticks = pdMS_TO_TICKS(timout.count());
    bool rc = xSemaphoreTake(m_semaphore, ticks);
    xSemaphoreGive(m_semaphore);
#ifdef ENABLE_SEM_DEBUG
    if (debug)
        DEBUG_SEM(TAG, "<< wait: Semaphore released: %s", toString().c_str());
#endif
    return rc;
} // wait

Semaphore::Semaphore(const char *name, uint8_t n)
{
    m_usePthreads = false; // Are we using pThreads or FreeRTOS?
    debug = false;
    if (n)
    {
        m_semaphore = xSemaphoreCreateCounting(n, n);
    }
    m_semaphore = xSemaphoreCreateBinary();
    xSemaphoreGive(m_semaphore);
    taken = false;
#ifdef ENABLE_SEM_DEBUG
    if (name)
        m_name = name;
    m_owner = "<N/A>";
#endif
    m_value = 0;
}

Semaphore::~Semaphore()
{

    vSemaphoreDelete(m_semaphore);
}

/**
 * @brief Give a semaphore.
 * The Semaphore is given.
 */
bool Semaphore::unlock()
{
#ifdef ENABLE_SEM_DEBUG
    if (debug)
        DEBUG_SEM(TAG, "Semaphore giving: %s", toString().c_str());
#endif
    bool rc = xSemaphoreGive(m_semaphore) == pdTRUE;
    taken = false;
#ifdef ENABLE_SEM_DEBUG
    if (debug)
        m_owner = std::string("<N/A>");
#endif
    return rc;
} // Semaphore::give

/**
 * @brief Give a semaphore from an ISR.
 */
void IRAM_ATTR Semaphore::giveFromISR()
{
    BaseType_t higherPriorityTaskWoken;
    xSemaphoreGiveFromISR(m_semaphore, &higherPriorityTaskWoken);

} // giveFromISR

/**
 * @brief Take a semaphore.
 * Take a semaphore and wait indefinitely.
 * @param [in] owner The new owner (for debugging)
 * @return True if we took the semaphore.
 */
bool IRAM_ATTR  Semaphore::lock(const char *owner)
{
#ifdef ENABLE_SEM_DEBUG
    if (debug)
        DEBUG_SEM(TAG, "Semaphore taking: %s for %s", toString().c_str(), owner);
#endif
    taken = xSemaphoreTake(m_semaphore, portMAX_DELAY) == pdTRUE;
#ifdef ENABLE_SEM_DEBUG
    if (taken)
    {
        if (debug)
        {
            m_owner = owner;
            DEBUG_SEM(TAG, "Semaphore taken:  %s", toString().c_str());
        }
    }
    else
    {
        if (debug)
            LOGE(TAG, "Semaphore NOT taken:  %s", toString().c_str());
    }
#endif
    return taken;
}

bool Semaphore::lock()
{
    return lock("");
}

/**
 * @brief Take a semaphore.
 * Take a semaphore but return if we haven't obtained it in the given period of milliseconds.
 * @param [in] timeoutMs Timeout in milliseconds.
 * @param [in] owner The new owner (for debugging)
 * @return True if we took the semaphore.
 */
bool IRAM_ATTR Semaphore::lock(std::chrono::milliseconds timeout, const char *owner)
{
#ifdef ENABLE_SEM_DEBUG
    if (debug)
        DEBUG_SEM(TAG, "Semaphore taking: %s for %s", toString().c_str(), owner);
#endif
    TickType_t ticks = pdMS_TO_TICKS(timeout.count());
    taken = xSemaphoreTake(m_semaphore, ticks);
#ifdef ENABLE_SEM_DEBUG
    if (taken)
    {
        if (debug)
            DEBUG_SEM(TAG, "Semaphore taken:  %s", toString().c_str());
        m_owner = owner;
    }
    else
    {
        LOGE(TAG, "Semaphore  %s NOT taken by : %s ", toString().c_str(), owner);
    }
#endif
    return taken;
}

void Semaphore::ActivateDebug()
{
    debug = true;
}
void Semaphore::DeActivateDebug()
{
    debug = false;
}

/**
 * @brief Create a string representation of the semaphore.
 * @return A string representation of the semaphore.
 */
std::string Semaphore::toString()
{
    std::string stringStream;
#ifdef ENABLE_SEM_DEBUG
    stringStream += "name: ";
    stringStream += m_name;
    stringStream += " owner ";
    stringStream += m_owner;
#endif
    return stringStream.c_str();
} // toString

/**
 * @brief Set the name of the semaphore.
 * @param [in] name The name of the semaphore.
 */
void Semaphore::setName(std::string name)
{
    m_name = name;
} // setName

std::unique_ptr<SemaphoreN> SemaphoreN::CreateUniqueM(const char *tag, uint8_t n)
{
    return std::make_unique<SemaphoreN>(tag, n);
}

std::unique_ptr<Semaphore> Semaphore::CreateUnique(const char *tag)
{
    return std::make_unique<Semaphore>(tag);
}