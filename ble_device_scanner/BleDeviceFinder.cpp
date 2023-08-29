
#include "BleDeviceFinder.hpp"
#include <string>
#include <thread>
#include "esp_log.h"
#include <chrono>

BleDeviceFinder::BleDeviceFinder(BleDeviceFinder_DeviceFoundCb_t callback) : Task("BleDeviceFinder", 4096), cb(callback)
{
    BLEDevice::init("");
    pClient = BLEDevice::createClient();
    pBLEScan = BLEDevice::getScan();
    pBLEScan->setAdvertisedDeviceCallbacks(this);
    pBLEScan->setActiveScan(true);
    isBusy = false;
}

BleDeviceFinder::~BleDeviceFinder()
{
}

void BleDeviceFinder::AddDeviceMac(const std::string &device)
{
    this->knownAddresses.push_back(device);
}

void BleDeviceFinder::notifyCallback(BLERemoteCharacteristic *pBLERemoteCharacteristic, uint8_t *pData, size_t length, bool isNotify)
{
    const std::string data = std::string((char *)pData, length);
    ESP_LOGI("BleDeviceFinder", "notifyCallback (isNotify %d) %s \n data=%s", isNotify, pBLERemoteCharacteristic->getUUID().toString().c_str(), data.c_str());
}

void BleDeviceFinder::onResult(BLEAdvertisedDevice Device)
{
    ESP_LOGI("BleDeviceFinder", "onResult  device mac %s , rssi =%d ", Device.getAddress().toString().c_str(), Device.getRSSI());
    bool known = false;
    for (const auto &addresse : knownAddresses)
    {
        if (strcmp(Device.getAddress().toString().c_str(), addresse.c_str()) == 0)
            known = true;
    }
    if (known)
    {
        ESP_LOGI("BleDeviceFinder", "device is match : mac %s , rssi =%d ", Device.getAddress().toString().c_str(), Device.getRSSI());
        if (this->cb != nullptr)
        {
            this->cb(Device);
        }
        Device.getScan()->stop();
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

void BleDeviceFinder::run(void *arg)
{
    while (!stopFlag)
    {
        isBusy = true;
        pBLEScan->start(5);
        isBusy = false;
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

esp_err_t BleDeviceFinder::StartScanning()
{
    stopFlag = false;
    return this->StartTask();
}
esp_err_t BleDeviceFinder::StopScanning()
{
    stopFlag = false;
    int counter = 0;
    while (isBusy && counter++ <= 6)
    {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    this->StopTask(true);
    return ESP_OK;
}