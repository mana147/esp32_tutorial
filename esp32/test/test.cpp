#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>
#include <Arduino.h>

int scanTime = 5;
BLEScan *pBLEScan;

class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice advertisedDevice) {

    String str_name = advertisedDevice.getName().c_str();
    String str_add = advertisedDevice.getAddress().toString().c_str();
    int getRSSI = advertisedDevice.getRSSI();
    char *pHex = BLEUtils::buildHexData(nullptr, (uint8_t *)advertisedDevice.getManufacturerData().data(), advertisedDevice.getManufacturerData().length());

    Serial.println(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>");
    Serial.print("str_name: "); Serial.println(str_name);
    Serial.print("str_add : "); Serial.println(str_add);
    Serial.print("RSSI : "); Serial.println(getRSSI);
    Serial.print("ManufacturerData : "); Serial.println(pHex);

  }
};

void setup() {
  Serial.begin(115200);
  BLEDevice::init("BLE123");
}

void loop() {
  pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setActiveScan(true);
  pBLEScan->setInterval(100);
  pBLEScan->setWindow(99);

  BLEScanResults foundDevices = pBLEScan->start(scanTime, false);
  Serial.print("Devices found: ");
  Serial.println(foundDevices.getCount());
  Serial.println("Scan done!");

  pBLEScan->clearResults();
  pBLEScan->stop();

  delay(1000);
}