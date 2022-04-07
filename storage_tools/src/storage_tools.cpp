

#include "debug_tools.h"
#if (DEBUG_STORAGE == VERBOS)
#define LOG_STORAGE ESP_LOGI
#define LOG_STORAGE_V ESP_LOGI
#elif (DEBUG_STORAGE == INFO)
#define LOG_STORAGE ESP_LOGI
#define LOG_STORAGE_V ESP_LOGD
#else
#define LOG_STORAGE ESP_LOGD
#define LOG_STORAGE_V ESP_LOGV
#endif

#include "storage_tools.h"
#include "assertion_tools.h"

std::timed_mutex Storage::fsLock;
bool Storage::isBusy = false;

/**
 * @brief open file with assertions and checks
 * 
 * @param filedir 
 * @param mode 
 * @return FILE* 
 */
FILE *Storage::Open(const char *filedir, const char *mode)
{
    if (!isMounted) // assert
    {
        LOGE(TAG, "Failed ,sdcard  is not mounted ! ");
        return (NULL);
    }
    if (lastOpened != NULL) // assert
    {
        LOGE(TAG, "Failed , please close last opened file and retry");
        return (NULL);
    }
    FILE *f = NULL;
    std::string filePath = mountPoint; // create proper file path
    filePath += "/";
    filePath += filedir;
    LOG_STORAGE_V(TAG, "Opening file %s  for mode %s ....", filePath.c_str(), mode);
    f = fopen(filePath.c_str(), mode);
    if (f == NULL)
    {
        LOGE(TAG, "Failed to open file for %s ", mode);
        return (NULL);
    }
    else
    {
        LOG_STORAGE_V(TAG, "file %s is OPEN.", filedir);
        lastOpened = f; //store last opened
        return (f);
    }
}

/**
 * @brief creat new file instance : dont forget to use delete to destroy the file 
 * 
 * @param filedir (ffile name or path without mounting point)
 * @return File_2* pointer to an onject of type File_2
 */
File_2 *Storage::OpenFile_P(const char *filedir)
{
    std::string filePath = this->mountPoint; // create proper file path
    filePath += "/";
    filePath += filedir;
    LOG_STORAGE(TAG, "returning file %s  ", filedir);
    return new File_2(filePath);
}

/**
 * @brief creat new file instance : dont forget to use delete to destroy the file 
 * 
 * @param filedir (ffile name or path without mounting point)
 * @return File_2* pointer to an onject of type File_2
 */
File_2 *Storage::OpenFile_P(const std::string &filedir)
{
    std::string filePath = this->mountPoint; // create proper file path
    filePath += "/";
    filePath += filedir;
    LOG_STORAGE_V(TAG, "returning file %s  ", filedir.c_str());
    return new File_2(filePath);
}

/**
 * @brief creat new file instance : dont forget to use delete to destroy the file 
 * 
 * @param filedir (ffile name or path without mounting point)
 * @return File_2* pointer to an onject of type File_2
 */
File_2 Storage::OpenFile(const std::string &filedir)
{
    std::string filePath = mountPoint; // create proper file path
    filePath += "/";
    filePath += filedir;
    LOG_STORAGE_V(TAG, "returning file %s  ", filedir.c_str());
    return File_2(filePath);
}

/**
 * @brief closef file and  assert for errors
 * 
 * @param file 
 * @return esp_err_t 
 */
esp_err_t Storage::Close()
{
    ASSERT_TRUE_WITH_MSG_AND_RETRUN_FAIL(isMounted, " Failed, SdCard is not mounted !");
    if (lastOpened != NULL) // an open file is found : close it !
    {
        int err = fclose(lastOpened);
        if (err == 0)
        {
            lastOpened = NULL;
            LOG_STORAGE_V(TAG, "File_2 closed with OK");
            return (ESP_OK);
        }
        else
        {
            lastOpened = NULL;
            LOGE(TAG, "Failed to close file or no changes were made to file %d ", err);
            return (ESP_FAIL);
        }
    }
    {
        LOGW(TAG, " file not valid ! what are you doing ?");
        return (ESP_OK);
    }
}

/**
 * @brief returns true if mounted (little_FS api call)
 * 
 * @return true 
 * @return false 
 */
bool Storage::IsMounted()
{
    return isMounted;
}

