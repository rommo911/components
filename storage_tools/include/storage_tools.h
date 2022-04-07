#ifndef ____STORAGE_H__
#define ____STORAGE_H__
#pragma once
#include <string>
#include <sys/stat.h>
#include "esp_err.h"
#include "esp_log.h"
#include "File_2.h"
#include <mutex>
class Storage
{

protected:
    static bool isBusy;
    static std::string activePartition;
    static std::timed_mutex fsLock;
    char TAG[20];
    size_t total;
    size_t used;
    std::string mountPoint;
    bool isMounted;
    virtual esp_err_t Mount() = 0;
    virtual esp_err_t Unmount() = 0; // pure virtal // force inherit

public:
    Storage()
    {
    }
    virtual ~Storage(){};
    const std::string&  GetMountPoint() const
    {
        return mountPoint;
    }
    virtual uint64_t GetFreeSpace() { return 0; };
    virtual esp_err_t format(const char *name) { return ESP_FAIL; };
    virtual bool IsMounted();
    esp_err_t Close();

    FILE *Open(const char *filedir, const char *mode);
    File_2 *OpenFile_P(const char *filedir);
    File_2 *OpenFile_P(const std::string &filedir);
    File_2 OpenFile(const std::string &filedir);
    FILE *lastOpened;
};

#endif // ____STORAGE_H__