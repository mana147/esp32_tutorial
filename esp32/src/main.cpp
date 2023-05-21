
#include <WiFi.h>
// Bluetooth LE
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>

#include "time.h"

// const char *ssid = "VCCorp";
// const char *password = "Vcc123**";

const char *ssid = "mmdholdings801";
const char *password = "mmdholdings801";

const char *ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 3600;
const int daylightOffset_sec = 3600;

// We collect each device MAC and RSSI
typedef struct
{
	char address[17]; // 67:f1:d2:04:cd:5d
	int rssi;
} BeaconData;

// Scan time must be longer than beacon interval
int beaconScanTime = 4;
uint8_t bufferIndex = 0; // Found devices counter
BeaconData buffer[10];	 // Buffer to store found device data
// ------------------------------------------------------------------------

void printLocalTime()
{
	delay(1000);
	struct tm timeinfo;
	if (!getLocalTime(&timeinfo))
	{
		Serial.println("Failed to obtain time");
		return;
	}
	Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
}

/*
 * Construct SenML compatible message with multiple measurements
 * see:
 * https://tools.ietf.org/html/draft-jennings-senml-10
 * Internal temp sensor:
 * https://github.com/pcbreflux/espressif/blob/master/esp32/arduino/sketchbook/ESP32_int_temp_sensor/ESP32_int_temp_sensor.ino
 */

class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks
{
public:
	void onResult(BLEAdvertisedDevice advertisedDevice)
	{
		extern uint8_t bufferIndex;
		extern BeaconData buffer[];
		if (bufferIndex >= 10)
		{
			return;
		}
		// RSSI
		if (advertisedDevice.haveRSSI())
		{
			buffer[bufferIndex].rssi = advertisedDevice.getRSSI();
		}
		else
		{
			buffer[bufferIndex].rssi = 0;
		}

		// MAC is mandatory for BT to work
		strcpy(buffer[bufferIndex].address, advertisedDevice.getAddress().toString().c_str());

		bufferIndex++;
		// Print everything via serial port for debugging
		Serial.printf("MAC: %s \n", advertisedDevice.getAddress().toString().c_str());
		Serial.printf("name: %s \n", advertisedDevice.getName().c_str());
		Serial.printf("RSSI: %d \n", advertisedDevice.getRSSI());
	}
};
void ScanBeacons()
{
	delay(1000);
	BLEScan *pBLEScan = BLEDevice::getScan(); // create new scan
	MyAdvertisedDeviceCallbacks cb;
	pBLEScan->setAdvertisedDeviceCallbacks(&cb);
	pBLEScan->setActiveScan(true); // active scan uses more power, but get results faster
	BLEScanResults foundDevices = pBLEScan->start(beaconScanTime);
	Serial.print("Devices found: ");

	for (uint8_t i = 0; i < bufferIndex; i++)
	{
		Serial.print(buffer[i].address);
		Serial.print(" : ");
		Serial.println(buffer[i].rssi);
	}

	// Stop BLE
	pBLEScan->stop();
	delay(1000);
	Serial.println("Scan done!");
}

void connectWiFi()
{
	WiFi.begin(ssid, password);
	while (WiFi.status() != WL_CONNECTED)
	{
		delay(500);
		Serial.println("Connecting to WiFi..");
	}
	Serial.println("Connected to the WiFi network");
}

void setup()
{
	Serial.begin(115200);
	WiFi.begin(ssid, password);

	while (WiFi.status() != WL_CONNECTED)
	{
		delay(1000);
		Serial.println("Connecting to WiFi...");
	}

	Serial.println("Connected to WiFi");

	// init and get the time
	configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
	BLEDevice::init("BLE"); // Can only be called onces
}

void loop()
{
	ScanBeacons();
	// Reconnect WiFi if not connected
	while (WiFi.status() != WL_CONNECTED)
	{
		connectWiFi();
	}

	printLocalTime();
}