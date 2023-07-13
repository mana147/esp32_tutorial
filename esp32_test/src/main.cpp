#include "config.h"

#include <Arduino.h>
#include <esp_task_wdt.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <Arduino_JSON.h>
#include <PubSubClient.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>
#include <ESP32Time.h>
#include "FS.h"
#include "SPIFFS.h"

typedef struct
{
	char name[40];
	char address[20];
	int rssi;
	char *data;
} BeaconData;
BeaconData bufferBeacons[num_device]; // Buffer to store found device data
uint8_t bufferIndex = 0;

BLEScan *pBLEScan;

class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks
{
public:
	void onResult(BLEAdvertisedDevice advertisedDevice)
	{
		extern uint8_t bufferIndex;
		extern BeaconData bufferBeacons[];

		if (bufferIndex >= num_device)
		{
			return;
		}

		// get Name
		if (advertisedDevice.haveName())
		{
			const char *name_beacon = advertisedDevice.getName().c_str();
			// bufferBeacons[bufferIndex].name = name;
			strcpy(bufferBeacons[bufferIndex].name, name_beacon);

			bufferBeacons[bufferIndex].rssi = advertisedDevice.getRSSI();

			bufferIndex++;
		}
	}
};

void ScanBeacons()
{
	pBLEScan = BLEDevice::getScan(); // create new scan

	BLEAdvertisedDeviceCallbacks *cb_handleDevice = new MyAdvertisedDeviceCallbacks();

	pBLEScan->setAdvertisedDeviceCallbacks(cb_handleDevice);
	pBLEScan->setActiveScan(true); // active scan uses more power, but get results faster
	pBLEScan->start(5, false);
	// Stop BLE
	pBLEScan->clearResults(); // delete results fromBLEScan buffer to release memory
	pBLEScan->stop();

	delete cb_handleDevice;
	delay(50);
}

String payloadJson_01(unsigned long &timestamp)
{
	// SenML begins
	String payloadString = " {\n ";
	payloadString += "\"name\":\"" ESP_NAME "\",\n";
	payloadString += "\"list_beacon_data\": [\n";

	for (uint8_t i = 0; i < bufferIndex; i++)
	{
		double distance = getCalculatedDistance(bufferBeacons[i].rssi);

		payloadString += "{\n";
		payloadString += "\"name\":\"" + String(bufferBeacons[i].name) + "\",\n";
		payloadString += "\"distance\":\"" + String(distance) + "\"\n";
		payloadString += "}\n";

		if (i < bufferIndex - 1)
		{
			payloadString += ",";
		}
	}

	payloadString += "],\n";
	payloadString += "\"time\":\"" + String(timestamp) + "\"\n";
	payloadString += "}";

	return payloadString;
}

// ----------------------------------------------------------
// ----------------------------------------------------------
// ----------------------------------------------------------

void setup()
{
	Serial.begin(115200);
	pinMode(LED, OUTPUT);

	BLEDevice::init("BLE");
}

void loop()
{
	Serial.println();
	digitalWrite(LED, LOW);

	// Scan Beacons
	ScanBeacons();

	for (int i = 0; i < num_device; i++)
	{
		String str_name = bufferBeacons[i].name;
		if (str_name.length() > 0)
		{
			Serial.println(str_name);
		}
	}

	// resset bufferIndex
	bufferIndex = 0;

	// for (int i = 0; i < 100; i++)
	// {
	// 	delete[] bufferBeacons[i].data;
	// 	delete bufferBeacons[i].name;
	// }

	// // Gán giá trị nullptr cho mảng
	// for (int i = 0; i < 100; i++)
	// {
	// 	bufferBeacons[i].data = nullptr;
	// }

	Serial.println("[APP] Free_heap_size: " + String(esp_get_free_heap_size()) + " bytes");

	digitalWrite(LED, HIGH);
	delay(100);
}
// ----------------------------------------------------------
// ----------------------------------------------------------
// ----------------------------------------------------------
