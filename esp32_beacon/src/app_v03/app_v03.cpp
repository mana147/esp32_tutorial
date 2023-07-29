#include "app_v03.h"
#include "app_v03.config.h"
#include <Arduino.h>

#include "BLEDevice.h"
#include "BLEUtils.h"
#include "BLEBeacon.h"
#include "esp_sleep.h"
#include "sys/time.h"

// -----------------------------------------------------------------------------------------
#define BEACON_UUID "a6948d76-adb7-42cf-941b-95bcabff34ad"
#define GPIO_DEEP_SLEEP_DURATION 10 // sleep x seconds and then wake up

RTC_DATA_ATTR static time_t last;        // remember last boot in RTC Memory
RTC_DATA_ATTR static uint32_t bootcount; // remember number of boots in RTC Memory

BLEAdvertising *pAdvertising; // BLE Advertisement type
struct timeval now;

// -----------------------------------------------------------------------------------------

void setBeacon()
{
    BLEBeacon oBeacon = BLEBeacon();
    oBeacon.setManufacturerId(0x4C00); // fake Apple 0x004C LSB (ENDIAN_CHANGE_U16!)
    oBeacon.setProximityUUID(BLEUUID(BEACON_UUID));
    oBeacon.setMajor((bootcount & 0xFFFF0000) >> 16);
    oBeacon.setMinor(bootcount & 0xFFFF);

    BLEAdvertisementData oAdvertisementData = BLEAdvertisementData();
    BLEAdvertisementData oScanResponseData = BLEAdvertisementData();
    oAdvertisementData.setFlags(0x04); // BR_EDR_NOT_SUPPORTED 0x04

    std::string strServiceData = "";
    strServiceData += (char)26;   // Len
    strServiceData += (char)0xFF; // Type
    strServiceData += oBeacon.getData();

    DEBUG_2("strServiceData :" , strServiceData.c_str());

    oAdvertisementData.addData(strServiceData);
    pAdvertising->setAdvertisementData(oAdvertisementData);
    pAdvertising->setScanResponseData(oScanResponseData);
}

// -----------------------------------------------------------------------------------------
App03::App03()
{
    // code
}

void App03::Setup()
{
    Serial.begin(115200);

    gettimeofday(&now, NULL);
    Serial.printf("start ESP32 %d\n", bootcount++);
    Serial.printf("deep sleep (%lds since last reset, %lds since last boot)\n", now.tv_sec, now.tv_sec - last);

    last = now.tv_sec;

    // Create the BLE Device
    BLEDevice::init("ESP32 as iBeacon");

    // Create the BLE Server
    BLEServer *pServer = BLEDevice::createServer(); // <-- no longer required to instantiate BLEServer, less flash and ram usage
    pAdvertising = BLEDevice::getAdvertising();
    BLEDevice::startAdvertising();

    setBeacon();

    // Start advertising
    pAdvertising->start();
    Serial.println("Advertizing started...");
    // delay(100);
    // pAdvertising->stop();
    // Serial.printf("enter deep sleep\n");
    // esp_deep_sleep(1000000LL * GPIO_DEEP_SLEEP_DURATION);
    // Serial.printf("in deep sleep\n");
}

void App03::Loop()
{
    // delay(100);
}

// -----------------------------------------------------------------------------------------