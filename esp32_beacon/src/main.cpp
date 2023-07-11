#include "config.h"

#include <sstream>
#include <string>
#include <typeinfo>
#include <algorithm>
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

// We collect each device MAC and RSSI
typedef struct
{
	String name;
	char address[17];
	int rssi;
	char *data;
} BeaconData;

uint8_t scanTimeBeacons = 3; // In seconds
uint8_t current_time = 0;

uint8_t bufferIndex = 0;			  // Found devices counter
BeaconData bufferBeacons[num_device]; // Buffer to store found device data
uint8_t message_char_buffer[MQTT_MAX_PACKET_SIZE];

ESP32Time RTC;
String array_name_device[num_device];
WiFiClient espClient;
PubSubClient clientMQTT(espClient);

// int _T;
// int scanTime = 2;	   // In seconds
// int scanInterval = 10; // In mili-seconds

// -----------------------------------------

template <class T>
String type_name(const T &)
{
	String s = __PRETTY_FUNCTION__;
	int start = s.indexOf("[with T = ") + 10;
	int stop = s.lastIndexOf(']');
	return s.substring(start, stop);
}

String combineStrings(const char *str1, const char *str2)
{
	String combinedString = String(str1) + String(str2);
	return combinedString;
}

void die()
{
	while (1)
	{
		delay(1000);
	}
}

// -----------------------------------------
// hàm này sử dụng thuật toán XOR để thực hiện mã hóa và giải mã.
// Mỗi ký tự trong chuỗi được XOR với ký tự tương ứng trong khóa,
// lặp lại khóa nếu chuỗi dài hơn khóa. Kết quả là một chuỗi được mã hóa hoặc giải mã.

String encrypt(const String &message, const String &key)
{
	String encrypted;
	for (size_t i = 0; i < message.length(); i++)
	{
		char c = message.charAt(i) ^ key.charAt(i % key.length());
		encrypted += c;
	}
	return encrypted;
}

String decrypt(const String &encrypted, const String &key)
{
	return encrypt(encrypted, key);
}

// -----------------------------------------

// tính toán khoảng cách
double getCalculatedDistance(double rssi)
{
	double referenceRssi = -65;
	double referenceDistance = 1;
	double flatFadingMitigation = 0;
	double pathLossExponent = 0.2;
	return referenceDistance * pow(10, -((rssi - referenceRssi - flatFadingMitigation) / 10) * pathLossExponent);
}

// kiểm trả chuỗi có trong chuỗi hay không
bool isTargetExist(String target, String arr[], int size)
{
	for (int i = 0; i < size; i++)
	{
		if (target == arr[i])
		{
			return true; // Nếu tìm thấy chuỗi "target" trong mảng
		}
	}
	return false; // Nếu không tìm thấy chuỗi "target" trong mảng
}

void printLocalTime()
{
	delay(50);
	struct tm timeinfo;
	if (!getLocalTime(&timeinfo))
	{
		Serial.println("Failed to obtain time");
		return;
	}
	Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
}

int connect2nearestAP()
{
	String hostname = "ESP32 Node";

	// this routine disconnects from WiFi, scans the network for the strongest AP at the defined SSID and tries to connect. returns WiFiOK, WiFiTimedOut, or WiFiNoSSID
	int n;
	int i = 0;

	// the following awkward code seems to work to avoid the following bug in ESP32 WiFi connection: https://github.com/espressif/arduino-esp32/issues/2501
	WiFi.disconnect(true); // delete old config
	WiFi.mode(WIFI_MODE_STA);
	WiFi.disconnect(true); // delete old config
	WiFi.setHostname(hostname.c_str());
	WiFi.begin();

	delay(SaveDisconnectTime); // 500ms seems to work in most cases, may depend on AP
	WiFi.disconnect(true);	   // delete old config

	Serial.println("scan start");
	n = WiFi.scanNetworks(); // WiFi.scanNetworks will return the number of networks found
	Serial.println("scan done");

	if (n == 0)
	{
		Serial.println("no networks found");
	}
	else
	{
		Serial.print(n);
		Serial.println(" networks found");
		for (int i = 0; i < n; ++i)
		{
			// Print SSID and RSSI for each network found
			Serial.print(i + 1);
			Serial.print(": ");
			Serial.print("BSSID: ");
			Serial.print(WiFi.BSSIDstr(i));
			Serial.print("  ");
			Serial.print(WiFi.RSSI(i));
			Serial.print("dBm, ");
			Serial.print(constrain(2 * (WiFi.RSSI(i) + 100), 0, 100));
			Serial.print("% ");
			Serial.print((WiFi.encryptionType(i) == WIFI_AUTH_OPEN) ? "open       " : "encrypted  ");
			Serial.println(WiFi.SSID(i));
			delay(10);
		}
	}
	Serial.println();

	while (String(ssid) != String(WiFi.SSID(i)) && (i < n))
	{
		i++;
	}

	if (i == n)
	{
		Serial.print("No network with SSID ");
		Serial.println(ssid);
		Serial.print(" found!");
		return WiFiNoSSID;
	}

	Serial.print("SSID match found at: ");
	Serial.println(i + 1);

	WiFi.setHostname(hostname.c_str());
	WiFi.begin(ssid, password, 0, WiFi.BSSID(i));
	i = 0;
	Serial.print("Connecting ");
	while ((WiFi.status() != WL_CONNECTED) && (i < WiFiTime * 10))
	{
		delay(100);
		Serial.print(".");
		i++;
	}

	Serial.println();

	if (WiFi.status() != WL_CONNECTED)
	{
		Serial.print("Connection to ");
		Serial.print(ssid);
		Serial.println(" timed out! ");
		return WiFiTimedOut;
	}

	Serial.print("Connected to BSSID: ");
	Serial.print(WiFi.BSSIDstr());
	Serial.print("  ");
	Serial.print(WiFi.RSSI());
	Serial.print("dBm, ");
	Serial.print(constrain(2 * (WiFi.RSSI() + 100), 0, 100));
	Serial.print("% ");
	Serial.print(WiFi.SSID());
	Serial.print(" IP: ");
	Serial.println(WiFi.localIP());

	return (WiFiOK);
}

void checkWiFi()
{
	// Check the signal strength of the connection - if too low, reconnect to the strongest AP;
	// good if the ESP is moving around and reconnection to the nearest AP makes sense
	int i = 1;
	if ((WiFi.status() == WL_CONNECTED) && (WiFi.RSSI() > WiFiRSSI) && (WiFi.RSSI() < -25))
	{ // good connection
		Serial.print("Connection good: ");
		Serial.print(WiFi.RSSI());
		Serial.print("dBm, ");
		Serial.print(constrain(2 * (WiFi.RSSI() + 100), 0, 100));
		Serial.print("% ");
		Serial.println(" ... keep geoing");
		return;
	}
	while (connect2nearestAP() != WiFiOK)
	{
		Serial.print("connect2nearestAP was called #");
		Serial.println(i);
		delay(100);
		i++;
		if (i > 10)
		{
			Serial.println("Rebooting cause WiFi setup failes");
			ESP.restart();
		}
	}
}

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
			String name = advertisedDevice.getName().c_str();

			// if (isTargetExist(name, array_name_device, sizeof(array_name_device) / sizeof(array_name_device[0])))
			if (true)
			{
				// Serial.print("device name :"); Serial.println(name);
				// Serial.println(" target exists in the array");

				bufferBeacons[bufferIndex].name = name;
				// Serial.print("device name :"); Serial.println(name);

				// RSSI
				if (advertisedDevice.haveRSSI())
				{
					bufferBeacons[bufferIndex].rssi = advertisedDevice.getRSSI();
					// Serial.print("device rssi :"); Serial.println(advertisedDevice.getRSSI());
				}

				// get data
				if (advertisedDevice.haveManufacturerData())
				{
					char *data = BLEUtils::buildHexData(nullptr, (uint8_t *)advertisedDevice.getManufacturerData().data(), advertisedDevice.getManufacturerData().length());
					bufferBeacons[bufferIndex].data = data;
				}

				// MAC is mandatory for BT to work
				const char *addr =  advertisedDevice.getAddress().toString().c_str();
				// Serial.print("device addr :"); Serial.println(addr);
				strcpy(bufferBeacons[bufferIndex].address, addr);

				bufferIndex++;
			}
		}
	}
};

void connectWiFi()
{
	WiFi.reconnect();
	while (WiFi.status() != WL_CONNECTED)
	{
		Serial.println("Connecting to WiFi..");
		delay(100);
	}
	Serial.println("Connected to the WiFi network");

	Serial.print("Connected to BSSID: ");
	Serial.print(WiFi.BSSIDstr());
	Serial.print("  ");
	Serial.print(WiFi.RSSI());
	Serial.print("dBm, ");
	Serial.print(constrain(2 * (WiFi.RSSI() + 100), 0, 100));
	Serial.print("% ");
	Serial.print(WiFi.SSID());
	Serial.print(" IP: ");
	Serial.println(WiFi.localIP());
}

void connectMQTT()
{
	clientMQTT.setServer(mqttServer, mqttPort);
	// clientMQTT.setBufferSize(1048);
	Serial.print("Connecting to MQTT...");
	if (clientMQTT.connect(ESP_NAME))
	{
		Serial.println("connected");
	}
	else
	{
		Serial.print("failed with state ");
		Serial.println(clientMQTT.state());
		delay(100);
	}
}

void ScanBeacons()
{

	BLEScan *pBLEScan = BLEDevice::getScan(); // create new scan
	pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
	pBLEScan->setActiveScan(true); // active scan uses more power, but get results faster

	BLEScanResults foundDevices = pBLEScan->start(scanTimeBeacons, false);

	// Stop BLE
	pBLEScan->clearResults(); // delete results fromBLEScan buffer to release memory
	pBLEScan->stop();
	delay(50);
}

void SendDataPOST(String data)
{
	WiFiClient client;
	HTTPClient http;

	const char *result = combineStrings(serverName, api_part_log).c_str();
	http.begin(client, result);
	http.addHeader("Content-Type", "application/json");
	http.addHeader("Authorization", c_recv_token); // Adding Bearer token as HTTP header

	// Send HTTP POST request
	int httpResponseCode = http.POST(data);
	Serial.print("HTTP Response code: ");
	Serial.println(httpResponseCode);

	// Free resources
	http.end();
}

void timeDataGET()
{
	HTTPClient http;
   
	const char *url_system_time = combineStrings(serverName, api_system_time).c_str();

	http.begin(url_system_time);
	http.addHeader("Content-Type", "application/json");
	http.addHeader("Authorization", c_recv_token); // Adding Bearer token as HTTP header

	// Send HTTP GET request
	int httpResponseCode = http.GET();

	if (httpResponseCode > 0)
	{
		Serial.print("HTTP Response code: ");
		Serial.println(httpResponseCode);
		String payload = http.getString();

		JSONVar myObject = JSON.parse(payload);
		// JSON.typeof(jsonVar) can be used to get the type of the var
		if (JSON.typeof(myObject) == "undefined")
		{
			Serial.println("Parsing input failed!");
			return;
		}

		if (myObject.hasOwnProperty("current_time"))
		{
			// Serial.print("myObject[\"current_time\"] = ");
			// Serial.println((int)myObject["current_time"]);
			current_time = (int)myObject["current_time"];
		}
	}

	// Free resources
	http.end();
}

void device_HTTP_GET()
{
	extern String array_name_device[];

	HTTPClient http;

	const char *url_api_v1_device = combineStrings(serverName, api_v1_device).c_str();

	http.begin(url_api_v1_device);
	http.addHeader("Content-Type", "application/json");
	http.addHeader("Authorization", c_recv_token);

	// Send HTTP GET request
	int httpResponseCode = http.GET();

	if (httpResponseCode > 0)
	{
		Serial.print("HTTP Response code: ");
		Serial.println(httpResponseCode);

		String payload = http.getString();
		JSONVar myObject = JSON.parse(payload);
		// Serial.println(myObject);

		if (JSON.typeof(myObject) == "undefined")
		{
			Serial.println("Parsing input failed!");
			return;
		}

		if (myObject.hasOwnProperty("current_time"))
		{
			// Serial.print("myObject[\"current_time\"] = ");
			// Serial.println((int)myObject["current_time"]);
			int current_time = myObject["current_time"];
			RTC.setTime(current_time);
		}

		if (myObject.hasOwnProperty("data"))
		{
			int leng_data = myObject["data"].length();

			for (int i = 0; i <= leng_data; i++)
			{
				String name = myObject["data"][i]["name"];
				// Serial.println(name);
				array_name_device[i] = name;
			}
		}
	}

	// Free resources
	http.end();
}

String payloadJson_01(unsigned long &timestamp)
{
	// SenML begins
	String payloadString = " {\n ";
	payloadString += "\"name\":\"" ESP_NAME "\",\n";
	payloadString += "\"device_id\":\"" ESP_NAME "\",\n";
	payloadString += "\"list_beacon_data\": [\n";

	for (uint8_t i = 0; i < bufferIndex; i++)
	{
		double distance = getCalculatedDistance(bufferBeacons[i].rssi);

		payloadString += "{\n";
		payloadString += "\"name\":\"" + String(bufferBeacons[i].name) + "\",\n";
		payloadString += "\"uuid\":\"" + String(bufferBeacons[i].data) + "\",\n";
		payloadString += "\"rssi\":\"" + String(bufferBeacons[i].rssi) + "\",\n";
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

String payloadJson_02(unsigned long &timestamp, double &distance, String name)
{
	String payload = "{\n";
	payload += "\"distance\": \"" + String(distance) + "\",\n";
	payload += "\"device_name\":\"" ESP_NAME "\",\n";
	payload += "\"beacon_name\":\"" + name + "\",\n";
	payload += "\"timestamp\":\"" + String(timestamp) + "\"\n";
	payload += "}";
	return payload;
}

// -----------------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------------

void setup()
{
	Serial.begin(115200);
	// connectWiFi();
	connect2nearestAP();
	// get list devices
	device_HTTP_GET();
	// Can only be called once
	BLEDevice::init("BLE");

	delay(100);
}

// -----------------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------------

void loop()
{
	Serial.println();
	Serial.println();
	boolean resultMQTT;

	// Scan Beacons
	ScanBeacons();
	Serial.println();

	Serial.print("bufferIndex : ");
	Serial.println(bufferIndex);

	unsigned long timestamp = RTC.getEpoch();
	Serial.print("timestamp : ");
	Serial.println(timestamp);

	// Reconnect WiFi if not connected
	Serial.print("wifi status : ");
	Serial.println(WiFi.status());
	while (WiFi.status() != WL_CONNECTED)
	{
		// connectWiFi();
		// checkWiFi();
		ESP.restart();
	}

	// Reconnect to MQTT if not connected
	Serial.print("status_clientMQTT : ");
	Serial.println(clientMQTT.connected());
	while (!clientMQTT.connected())
	{
		connectMQTT();
	}

	if (WiFi.status() == WL_CONNECTED)
	{

		// for (uint8_t i = 0; i < bufferIndex; i++)
		// {
		// 	Serial.println(String(bufferBeacons[i].data));
		// }

		String dataJson = payloadJson_01(timestamp);
		size_t byteCount = strlen(dataJson.c_str());

		// Serial.println(dataJson);
		Serial.println(byteCount);

		Serial.print("MQTT state: ");
		Serial.println(clientMQTT.state());

		resultMQTT = clientMQTT.publish(c_TOPIC, dataJson.c_str());

		// dataJson.getBytes(message_char_buffer, dataJson.length() + 1);
		// resultMQTT = clientMQTT.publish(c_TOPIC, message_char_buffer, dataJson.length(), false);

		Serial.print("MQTT Result: ");
		Serial.println(resultMQTT);
	}

	clientMQTT.loop();
	bufferIndex = 0;
	delay(100);
}

// -----------------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------------
