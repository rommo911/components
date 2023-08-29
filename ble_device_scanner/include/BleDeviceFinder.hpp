#ifndef __BLEDEVICEFINDER_H__
#define __BLEDEVICEFINDER_H__

#include "BLEDevice.h"
#include <functional>
#include "esp_err.h"
#include "FreeRTOS.hpp"
#include "system.hpp"
#include "task_tools.h"
#include <atomic>

typedef std::function<void(const BLEAdvertisedDevice& device)> BleDeviceFinder_DeviceFoundCb_t;

class BleDeviceFinder : public BLEAdvertisedDeviceCallbacks , private Task
{
private:
    BLEScan *pBLEScan;
    BLEClient *pClient;
    bool deviceFound = false;
    std::vector <std::string> knownAddresses = {};
    const BleDeviceFinder_DeviceFoundCb_t cb = nullptr;
    void onResult(BLEAdvertisedDevice Device);
    void run(void* arg);
    bool stopFlag = false;
    std::atomic<bool> isBusy;

public:
    BleDeviceFinder(BleDeviceFinder_DeviceFoundCb_t);
    ~BleDeviceFinder();
    static void notifyCallback( BLERemoteCharacteristic *pBLERemoteCharacteristic, uint8_t *pData, size_t length,bool isNotify);
    void AddDeviceMac(const std::string&);
    esp_err_t StartScanning();
    esp_err_t StopScanning();

};

#endif