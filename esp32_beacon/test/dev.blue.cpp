#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

BLECharacteristic *pCharacteristic;
bool deviceConnected = false;
uint8_t txValue = 0;

// See the following for generating UUIDs:
// https://www.uuidgenerator.net/

#define SERVICE_UUID "1b68cacd-2818-4a71-a49b-0061181c97a2"
#define CHARACTERISTIC_UUID_RX "582adc94-2eab-4735-b821-293883e26762"
#define CHARACTERISTIC_UUID_TX "0da69903-6d69-4810-bccc-4c56d2880188"


class MyServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer *pServer) {
    deviceConnected = true;
  };

  void onDisconnect(BLEServer *pServer) {
    deviceConnected = false;
  }
};

class MyCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic) {
    std::string rxValue = pCharacteristic->getValue();

    /*
       * my iOS app sends a 5 byte payload
       * so thisis how it is listened for
       */

    if (rxValue.length() == 5) {

      byte data0 = (uint8_t)rxValue[0];
      byte data1 = (uint8_t)rxValue[1];
      byte data2 = (uint8_t)rxValue[2];
      byte data3 = (uint8_t)rxValue[3];
      byte data4 = (uint8_t)rxValue[4];

      Serial.print(data0);
      Serial.print(", ");
      Serial.print(data1);
      Serial.print(", ");
      Serial.print(data2);
      Serial.print(", ");
      Serial.print(data3);
      Serial.print(", ");
      Serial.println(data4);
    }
  }
};


void setup() {
  Serial.begin(115200);

  // Create the BLE Device
  //BLEDevice::init("UART Service");
  BLEDevice::init("ESP123");
  /*
   * this appears to need to be 5 characters or less
   * when advertising the service UUID
   */


  // Create the BLE Server
  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // Create the BLE Service
  BLEService *pService = pServer->createService(SERVICE_UUID);

  // Create a BLE Characteristic
  pCharacteristic = pService->createCharacteristic(
    CHARACTERISTIC_UUID_TX,
    BLECharacteristic::PROPERTY_NOTIFY);

  pCharacteristic->addDescriptor(new BLE2902());

  BLECharacteristic *pCharacteristic = pService->createCharacteristic(
    CHARACTERISTIC_UUID_RX,
    BLECharacteristic::PROPERTY_WRITE
      | BLECharacteristic::PROPERTY_WRITE_NR);

  pCharacteristic->setCallbacks(new MyCallbacks());

  // Start the service
  pService->start();

  // Start advertising
  pServer->getAdvertising()->addServiceUUID(pService->getUUID());  // needed for RB compat.
  /*
   * need to advertise the service UUID for the redbear code to work 
   * as it looks for it
   */
  pServer->getAdvertising()->start();

  Serial.println("Waiting a client connection to notify...");
}

void loop() {

  if (deviceConnected) {
    Serial.printf("*** Sent Value: %d ***\n", txValue);
    pCharacteristic->setValue(&txValue, 1);
    pCharacteristic->notify();
    txValue++;
  }
  delay(1000);
}