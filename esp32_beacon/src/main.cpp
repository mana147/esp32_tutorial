#include <sstream>
#include <string>
#include <typeinfo>
#include <algorithm>
#include <TimeLib.h>

#include <WiFi.h>
#include <HTTPClient.h>
#include <Arduino_JSON.h>

#define MQTT_MAX_PACKET_SIZE 1844
#define num_device 100
#define ESP_NAME "esp_001_002"

#include <PubSubClient.h>
// Bluetooth LE
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>

const char *ssid = "VCCorp";
const char *password = "Vcc123**";

// const char *ssid = "P301";
// const char *password = "123456789";

const char *serverName = "http://phub.bctoyz.com";
const char *api_part_log = "/api/v1/map/log-devices";

const char *ntpServer = "pool.ntp.org"; // vn.pool.ntp.org
const long gmtOffset_sec = 3600;
const int daylightOffset_sec = 3600;

String arr_name[] = {"dps1", "dps2", "dps3", "R23040111", "R23040110", "R23040106-", "R23040107", "R23040108"};

/* MQTT credentials and connection */
const char *mqttServer = "127.0.0.1";
const int mqttPort = 1883;
const char *mqttUser = "esp32_001_001";
const char *mqttPassword = "123vcc";

// Scan time must be longer than beacon interval

int beaconScanTime = 5;
WiFiClient espClient;
PubSubClient clientMQTT(espClient);

// We collect each device MAC and RSSI
typedef struct
{
	String name;
	char address[17];
	int rssi;
	char *data;
} BeaconData;

int current_time = 0;
uint8_t bufferIndex = 0;			  // Found devices counter
BeaconData bufferBeacons[num_device]; // Buffer to store found device data
uint8_t message_char_buffer[MQTT_MAX_PACKET_SIZE];

// -----------------------------------------

template <class T>
String type_name(const T &)
{
	String s = __PRETTY_FUNCTION__;
	int start = s.indexOf("[with T = ") + 10;
	int stop = s.lastIndexOf(']');
	return s.substring(start, stop);
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

// -----------------------------------------

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

			name.trim();

			if (isTargetExist(name, arr_name, sizeof(arr_name) / sizeof(arr_name[0])))
			{
				// Serial.print(name);
				// Serial.println(" target exists in the array");

				bufferBeacons[bufferIndex].name = name;

				// RSSI
				if (advertisedDevice.haveRSSI())
				{
					bufferBeacons[bufferIndex].rssi = advertisedDevice.getRSSI();
				}

				// get data
				if (advertisedDevice.haveManufacturerData())
				{
					bufferBeacons[bufferIndex].data = BLEUtils::buildHexData(nullptr, (uint8_t *)advertisedDevice.getManufacturerData().data(), advertisedDevice.getManufacturerData().length());
				}

				// MAC is mandatory for BT to work
				strcpy(bufferBeacons[bufferIndex].address, advertisedDevice.getAddress().toString().c_str());

				bufferIndex++;
			}
		}
	}
};

class mCallbacks : public BLEAdvertisedDeviceCallbacks
{
	void onResult(BLEAdvertisedDevice advertisedDevice)
	{
		// Serial.printf("Advertised Device: %s \n", advertisedDevice.toString().c_str());
		// Serial.printf("%s \n", advertisedDevice.toString());

		String str_name = advertisedDevice.getName().c_str();
		int getRSSI = advertisedDevice.getRSSI();
		char *pHex = BLEUtils::buildHexData(nullptr, (uint8_t *)advertisedDevice.getManufacturerData().data(), advertisedDevice.getManufacturerData().length());

		Serial.println("------------------------------");
		Serial.println(str_name);
		Serial.println(getRSSI);
		Serial.println(pHex);
		Serial.println("------------------------------");
	}
};

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

void connectMQTT()
{
	clientMQTT.setServer(mqttServer, mqttPort);
	Serial.println("Connecting to MQTT...");
	if (clientMQTT.connect("ESP32Client", mqttUser, mqttPassword))
	{
		Serial.println("connected");
	}
	else
	{
		Serial.print("failed with state ");
		Serial.print(clientMQTT.state());
		delay(2000);
	}
}

void ScanBeacons()
{
	BLEScan *pBLEScan = BLEDevice::getScan(); // create new scan
	pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
	pBLEScan->setActiveScan(true); // active scan uses more power, but get results faster
	pBLEScan->setInterval(100);
	pBLEScan->setWindow(99);

	BLEScanResults foundDevices = pBLEScan->start(beaconScanTime);
	// Serial.print("Devices found: ");
	// Serial.println(foundDevices.getCount());

	// Stop BLE
	pBLEScan->clearResults(); // delete results fromBLEScan buffer to release memory
	pBLEScan->stop();
	delay(500);
}

void SendDataPOST(String data)
{
	WiFiClient client;
	HTTPClient http;

	char result[100];

	// Ghép hai chuỗi vào nhau
	strcpy(result, serverName);
	strcat(result, api_part_log);

	// Your Domain name with URL path or IP address with path
	http.begin(client, result);

	String recv_token = "1|p02XlC16ykTHCnKRpQKM5iF6uijShnzqwtNE9h35"; // Complete Bearer token
	recv_token = "Bearer " + recv_token;							  // Adding "Bearer " before token
	http.addHeader("Content-Type", "application/json");
	http.addHeader("Authorization", recv_token); // Adding Bearer token as HTTP header

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

	String serverPath = "http://phub.bctoyz.com/api/v1/map/system-time";

	// Your Domain name with URL path or IP address with path
	http.begin(serverPath.c_str());

	String recv_token = "1|p02XlC16ykTHCnKRpQKM5iF6uijShnzqwtNE9h35"; // Complete Bearer token
	recv_token = "Bearer " + recv_token;

	http.addHeader("Content-Type", "application/json");
	http.addHeader("Authorization", recv_token); // Adding Bearer token as HTTP header

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
	HTTPClient http;

	String serverPath = "http://phub.bctoyz.com/api/v1/device";
	http.begin(serverPath.c_str());

	String recv_token = "Bearer 1|p02XlC16ykTHCnKRpQKM5iF6uijShnzqwtNE9h35";
	http.addHeader("Content-Type", "application/json");
	http.addHeader("Authorization", recv_token); // Adding Bearer token as HTTP header

	// Send HTTP GET request
	int httpResponseCode = http.GET();

	if (httpResponseCode > 0)
	{
		Serial.print("HTTP Response code: ");
		Serial.println(httpResponseCode);
		String payload = http.getString();

		JSONVar myObject = JSON.parse(payload);

		if (JSON.typeof(myObject) == "undefined")
		{
			Serial.println("Parsing input failed!");
			return;
		}

		if (myObject.hasOwnProperty("current_time"))
		{
			Serial.print("myObject[\"current_time\"] = ");
			Serial.println((int)myObject["current_time"]);
		}

		if (myObject.hasOwnProperty("data"))
		{
			Serial.print("myObject[\"data\"] = ");
			Serial.println(myObject["data"]);
			Serial.println(myObject["data"][0]);

			// int leng_data = myObject["data"].length();
		}
	}

	// Free resources
	http.end();
}

// -----------------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------------

void setup()
{
	Serial.begin(115200);

	connectWiFi();

	// get list devices
	device_HTTP_GET();

	while (true)
	{
		// Serial.print(".");
		delay(1000);
	}

	// // get time
	// // timeDataGET();
	// // setTime(current_time);

	// Can only be called once
	BLEDevice::init("BLE");

	// init and get the time
	// configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
}

// -----------------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------------

void loop()
{

	// Scan Beacons
	ScanBeacons();

	// Reconnect WiFi if not connected
	while (WiFi.status() != WL_CONNECTED)
	{
		connectWiFi();
	}

	if (WiFi.status() == WL_CONNECTED)
	{

		// SenML begins
		String httpRequestData = "{";
		httpRequestData += "device_id: " ESP_NAME ",";
		httpRequestData += "list_beacon_data: [";

		for (uint8_t i = 0; i < bufferIndex; i++)
		{
			httpRequestData += "{";
			httpRequestData += "name: " + String(bufferBeacons[i].name) + ",";
			httpRequestData += "manu: 4c000216,";
			httpRequestData += "uuid: " + String(bufferBeacons[i].data) + ",";
			httpRequestData += "rssi: " + String(bufferBeacons[i].rssi);
			httpRequestData += "}";

			if (i < bufferIndex - 1)
			{
				httpRequestData += ",";
			}
		}

		httpRequestData += "],";
		httpRequestData += "time:" + String(1685605688);
		httpRequestData += "}";
		Serial.println(httpRequestData);

		SendDataPOST(httpRequestData);
	}

	// Start over the scan loop
	bufferIndex = 0;
	// Add delay to slow down publishing frequency if needed.
	delay(500);
}

// -----------------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------------
