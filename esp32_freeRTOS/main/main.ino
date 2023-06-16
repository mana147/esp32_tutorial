#include <sstream>
#include <string>
#include <typeinfo>
#include <algorithm>
#include <TimeLib.h>
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

#define MQTT_MAX_PACKET_SIZE 1844
#define num_device 100
#define ESP_NAME "esp_001_003"

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

WiFiClient espClient;
PubSubClient clientMQTT(espClient);

int _T;
int scanTime = 2;	   // In seconds
int scanInterval = 10; // In mili-seconds

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

String array_name_device[100];

ESP32Time rtc;

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

#define MaxSecWiFi 30			// max time in s for WiFi setup, function will return WiFiErrorCode if WiFi was not successful.
#define WiFiErrorCode 1000		// see above
#define SaveDisconnectTime 1000 // Time im ms for save disconnection, needed to avoid that WiFi works only evey secon boot: https://github.com/espressif/arduino-esp32/issues/2501
#define WiFiTime 100			// Max time in s for successful WiFi connection.
#define WiFiOK 0				// WiFi connected
#define WiFiTimedOut 1			// WiFi connection timed out
#define WiFiNoSSID 2			// The SSID was not found during network scan
#define WiFiRSSI -75			// Min. required signal strength for a good WiFi cennection. If lower, new scan is initiated in checkWiFi()

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
	// Check the signal strength of the connection - if too low, reconnect to the strongest AP; good if the ESP is moving around and reconnection to the nearest AP makes sense
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
		delay(1000);
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

			name.trim();

			Serial.print("name : ");
			Serial.println(name);

			if (isTargetExist(name, array_name_device, sizeof(array_name_device) / sizeof(array_name_device[0])))
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
	WiFi.disconnect();
	// WiFi.begin(ssid, password);
	WiFi.reconnect();
	while (WiFi.status() != WL_CONNECTED)
	{
		delay(500);
		Serial.println("Connecting to WiFi..");
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
	int scanTime = 5 ; //In seconds

	BLEScan *pBLEScan = BLEDevice::getScan(); // create new scan
	pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
	pBLEScan->setActiveScan(true); // active scan uses more power, but get results faster

	BLEScanResults foundDevices = pBLEScan->start(scanTime, false);

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
	extern String array_name_device[];

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

			int current_time = myObject["current_time"];
			rtc.setTime(current_time);
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

	delay(500);
}

// -----------------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------------

void loop()
{

	unsigned long timestamp = rtc.getEpoch();

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
		String httpRequestData = "{\n";
		httpRequestData += "\"name\":\"" ESP_NAME "\",\n";
		httpRequestData += "\"device_id\":\"" ESP_NAME "\",\n";
		httpRequestData += "\"type\":\"hub\",\n";
		httpRequestData += "\"list_beacon_data\": [\n";

		for (uint8_t i = 0; i < bufferIndex; i++)
		{
			httpRequestData += "{\n";
			httpRequestData += "\"name\":\"" + String(bufferBeacons[i].name) + "\",\n";
			httpRequestData += "\"manu\":\"4c000215\",\n";
			httpRequestData += "\"uuid\": \"" + String(bufferBeacons[i].data) + "\",\n";
			httpRequestData += "\"rssi\":\"" + String(bufferBeacons[i].rssi) + "\"\n";
			httpRequestData += "}\n";

			if (i < bufferIndex - 1)
			{
				httpRequestData += ",";
			}
		}

		httpRequestData += "],\n";
		httpRequestData += "\"time\":\"" + String(timestamp) + "\"\n";
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
