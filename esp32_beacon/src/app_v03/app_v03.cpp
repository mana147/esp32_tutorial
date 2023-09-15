#include "app_v03.h"
#include "app_v03.config.h"
#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>

// -----------------------------------------------------------------------------------------
int scanTime = 5; // In seconds
BLEScan *p_BLEScan;
// -----------------------------------------------------------------------------------------

class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks
{
public:
    void onResult(BLEAdvertisedDevice advertisedDevice)
    {
        // extern uint8_t bufferIndex;
        // extern BeaconData bufferBeacons[];

        // Serial.print("bufferIndex");
        // Serial.println(bufferIndex);

        Serial.println(advertisedDevice.getName().c_str());

    }
};
// -----------------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------------

void ScanBLE()
{
    BLEAdvertisedDeviceCallbacks *cb_handleDevice = new MyAdvertisedDeviceCallbacks();

    p_BLEScan = BLEDevice::getScan(); // create new scan
    p_BLEScan->setAdvertisedDeviceCallbacks(cb_handleDevice);
    p_BLEScan->setActiveScan(true); // active scan uses more power, but get results faster
    p_BLEScan->start(5, false);
    delay(100);
    p_BLEScan->clearResults(); // delete results fromBLEScan buffer to release memory
    p_BLEScan->stop();

    delete cb_handleDevice;
    delay(100);
}
// -----------------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------------

void App03::Setup()
{
    Serial.begin(115200);
    Serial.println("Scanning...");

    BLEDevice::init("BLE");
    delay(100);
}
// -----------------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------------

void App03::Loop()
{
    ScanBLE();
    delay(100);
}

// -----------------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------------
App03::App03()
{
}