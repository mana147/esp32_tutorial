#include "app_v01.h"
#include "app_v01.config.h"
#include <Arduino.h>
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

// We collect each device MAC and RSSI
typedef struct
{
    char name[40];
    char address[20];
    int rssi;
} BeaconData;
BeaconData bufferBeacons[num_device];

uint32_t u32t_scanTimeBeacons = 3; // In seconds
uint16_t u16t_MQTT_PACKET_SIZE = 2048;
uint8_t current_time = 0;
uint8_t bufferIndex = 0; // Found devices counter
uint8_t message_char_buffer[MQTT_MAX_PACKET_SIZE];
String array_name_device[num_device];

ESP32Time RTC;
WiFiClient espClient;
PubSubClient clientMQTT(espClient);
BLEScan *pBLEScan;

#define die   \
    while (1) \
        delay(1000);

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

String read_files_in_SPIFFS(String string)
{
    String str;
    File data_files;
    data_files = SPIFFS.open(string, "r");
    if (data_files)
    {
        while (data_files.available())
        {
            str = data_files.readString();
            data_files.flush();
        }
        data_files.close();
    }
    return str;
}

int write_files_in_SPIFFS(String string, String path)
{
    int key;
    File data_flies;
    data_flies = SPIFFS.open(path, "w");
    if (data_flies)
    {
        data_flies.print(string);
        data_flies.flush();
        key = 1;
    }
    else
    {
        key = 0;
    }

    data_flies.close();
    return key;
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
        DEBUG_2("c: ", c);
        encrypted += c;
    }
    return encrypted;
}

String decrypt(const String &encrypted, const String &key)
{
    return encrypt(encrypted, key);
}

void encryptXOR(char *message, size_t messageLength, const char *key, size_t keyLength)
{
    for (size_t i = 0; i < messageLength; i++)
    {
        message[i] = message[i] ^ key[i % keyLength];
    }
}

void decryptXOR(char *encrypted, size_t encryptedLength, const char *key, size_t keyLength)
{
    encryptXOR(encrypted, encryptedLength, key, keyLength);
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
        DEBUG_1("Failed to obtain time");
        return;
    }
    DEBUG_2(&timeinfo, "%A, %B %d %Y %H:%M:%S");
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
    WiFi.disconnect(true);     // delete old config

    DEBUG_1("scan start");
    n = WiFi.scanNetworks(); // WiFi.scanNetworks will return the number of networks found
    DEBUG_1("scan done");

    if (n == 0)
    {
        DEBUG_1("no networks found");
    }
    else
    {
        DEBUG_2(n, " networks found")
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
    DEBUG_1("");

    while (String(ssid) != String(WiFi.SSID(i)) && (i < n))
    {
        i++;
    }

    if (i == n)
    {
        DEBUG_2("No network with SSID ", ssid);

        digitalWrite(LED, HIGH);
        delay(3000);
        digitalWrite(LED, LOW);

        ESP.restart();

        return WiFiNoSSID;
    }

    DEBUG_2("SSID match found at: ", i + 1);

    WiFi.setHostname(hostname.c_str());
    WiFi.begin(ssid, password, 0, WiFi.BSSID(i));
    i = 0;
    DEBUG_1("Connecting ");
    while ((WiFi.status() != WL_CONNECTED) && (i < WiFiTime * 10))
    {
        digitalWrite(LED, HIGH);
        delay(100);
        digitalWrite(LED, LOW);
        delay(100);
        Serial.print(".");
        i++;
    }
    digitalWrite(LED, LOW);

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
        DEBUG_2("connect2nearestAP was called #", 1);
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
        // extern uint8_t bufferIndex;
        // extern BeaconData bufferBeacons[];

        // Serial.print("bufferIndex");
        // Serial.println(bufferIndex);

        if (bufferIndex >= num_device)
        {
            return;
        }

        // check device have name
        if (advertisedDevice.haveName())
        {
            // get device name
            const char *name_beacon = advertisedDevice.getName().c_str();

            //if (isTargetExist(advertisedDevice.getName().c_str(), array_name_device, sizeof(array_name_device) / sizeof(array_name_device[0])))
            if (true)
            {
                // get device name
                strcpy(bufferBeacons[bufferIndex].name, name_beacon);

                // get device RSSI
                bufferBeacons[bufferIndex].rssi = advertisedDevice.getRSSI();

                // get device MAC is mandatory
                const char *addrMac = advertisedDevice.getAddress().toString().c_str();
                strcpy(bufferBeacons[bufferIndex].address, addrMac);

                bufferIndex++;
            }

            // if (isTargetExist(name_beacon, array_name_device, sizeof(array_name_device) / sizeof(array_name_device[0])))

            // get data
            // if (advertisedDevice.haveManufacturerData())
            // {
            // 	char *data = BLEUtils::buildHexData(nullptr, (uint8_t *)advertisedDevice.getManufacturerData().data(), advertisedDevice.getManufacturerData().length());
            // 	bufferBeacons[bufferIndex].data = data;
            // }
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
    clientMQTT.setBufferSize(u16t_MQTT_PACKET_SIZE);
    Serial.print("Connecting to MQTT...");
    if (clientMQTT.connect(ESP_NAME, mqttUser, mqttPassword))
    {
        Serial.println("connected");
    }
    else
    {
        Serial.print("failed with state ");
        Serial.println(clientMQTT.state());
    }
    delay(50);
}

void ScanBeacons()
{
    BLEAdvertisedDeviceCallbacks *cb_handleDevice = new MyAdvertisedDeviceCallbacks();

    pBLEScan = BLEDevice::getScan(); // create new scan
    pBLEScan->setAdvertisedDeviceCallbacks(cb_handleDevice);
    pBLEScan->setActiveScan(true); // active scan uses more power, but get results faster
    pBLEScan->start(u32t_scanTimeBeacons, false);
    delay(50);
    pBLEScan->clearResults(); // delete results fromBLEScan buffer to release memory
    pBLEScan->stop();

    delete cb_handleDevice;
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

void handle_data_json(String payload)
{
    extern String array_name_device[];

    JSONVar myObject = JSON.parse(payload);

    if (JSON.typeof(myObject) == "undefined")
    {
        Serial.println("Parsing input failed!");
        return;
    }

    if (myObject.hasOwnProperty("current_time"))
    {
        int current_time = myObject["current_time"];
        RTC.setTime(current_time);
    }

    if (myObject.hasOwnProperty("data"))
    {
        int leng_data = myObject["data"].length();

        for (int i = 0; i <= leng_data; i++)
        {
            String name = myObject["data"][i]["name"];
            array_name_device[i] = name;
        }
    }
}

void device_HTTP_GET()
{
    Serial.println("begin http get list device ...... ");

    HTTPClient http;

    const char *url = combineStrings(serverName, api_v1_device_beacon).c_str();

    http.begin(url);
    http.addHeader("Content-Type", "application/json");
    http.addHeader("Authorization", c_recv_token);

    // Send HTTP GET request
    int httpResponseCode = http.GET();

    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);

    if (httpResponseCode == 200)
    {
        Serial.println("> get list data beacon from server");
        String payload = http.getString();
        handle_data_json(payload);
    }
    else
    {
        Serial.println("> get list data beacon from SPIFFS");
        String data_txt = read_files_in_SPIFFS(c_PATH_FILE_TXT);
        handle_data_json(data_txt);
    }

    // Free resources
    http.end();
}

// -----------------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------------

String payloadJson_01(unsigned long &timestamp)
{
    // SenML begins
    String payloadString = "{";
    payloadString += "\"name\":\"" ESP_NAME "\",";
    payloadString += "\"list_beacon_data\": [";

    for (uint8_t i = 0; i < bufferIndex; i++)
    {
        double distance = getCalculatedDistance(bufferBeacons[i].rssi);

        payloadString += "{";
        payloadString += "\"name\":\"" + String(bufferBeacons[i].name) + "\",";
        payloadString += "\"address\":\"" + String(bufferBeacons[i].address) + "\",";
        payloadString += "\"distance\":\"" + String(distance) + "\"";
        payloadString += "}";

        if (i < bufferIndex - 1)
        {
            payloadString += ",";
        }
    }

    payloadString += "],";
    payloadString += "\"time\":\"" + String(timestamp) + "\",";
    payloadString += "\"heap_size\":\"" + String(esp_get_free_heap_size()) + "\"";
    payloadString += "}";

    return payloadString;
}

String payloadJson_02(unsigned long &timestamp, double &distance, String name)
{
    String payload = "{";
    payload += "\"distance\": \"" + String(distance) + "\",";
    payload += "\"device_name\":\"" ESP_NAME "\",";
    payload += "\"beacon_name\":\"" + name + "\",";
    payload += "\"timestamp\":\"" + String(timestamp) + "\"";
    payload += "}";
    return payload;
}
// --------------------------------------------------------------------------
void appendToString(char *str, const char *toAppend)
{
    strcat(str, toAppend);
}

void appendToString(char *str, int num)
{
    char numStr[20];
    sprintf(numStr, "%d", num);
    strcat(str, numStr);
}

void appendToString(char *str, unsigned long num)
{
    char numStr[20];
    sprintf(numStr, "%lu", num);
    strcat(str, numStr);
}

void appendToString(char *str, double num)
{
    char numStr[10];
    dtostrf(num, 6, 2, numStr);
    strcat(str, numStr);
}

// void appendToString(char *str, uint32_t num)
// {
//     char numStr[20];
//     sprintf(numStr, "%lu", num);
//     strcat(str, numStr);
// }

void char_payloadJson(unsigned long &timestamp, char *payloadString)
{
    // SenML begins
    strcpy(payloadString, "{");
    appendToString(payloadString, "\"name\":\"" ESP_NAME "\",");
    appendToString(payloadString, "\"list_beacon_data\": [");

    for (uint8_t i = 0; i < bufferIndex; i++)
    {
        double distance = getCalculatedDistance(bufferBeacons[i].rssi);

        appendToString(payloadString, "{");
        appendToString(payloadString, "\"name\":\"");
        appendToString(payloadString, bufferBeacons[i].name);
        appendToString(payloadString, "\",");
        appendToString(payloadString, "\"address\":\"");
        appendToString(payloadString, bufferBeacons[i].address);
        appendToString(payloadString, "\",");
        appendToString(payloadString, "\"distance\":\"");
        appendToString(payloadString, distance);
        appendToString(payloadString, "\"");
        appendToString(payloadString, "}");

        if (i < bufferIndex - 1)
        {
            appendToString(payloadString, ",");
        }
    }

    // int heap_size = esp_get_free_heap_size();
    appendToString(payloadString, "],");
    appendToString(payloadString, "\"time\":\"");
    appendToString(payloadString, timestamp);
    // appendToString(payloadString, "\",");
    // appendToString(payloadString, "\"heap_size\":\"");
    // appendToString(payloadString, heap_size);
    appendToString(payloadString, "\"");
    appendToString(payloadString, "}");
}

// --------------------------------------------------------------------------

bool optionMODE()
{
    bool ck;
    int time_check = 1;
    while (true)
    {
        int check_touchRead = touchRead(PIN_NUMB_TOUCH);

        if (check_touchRead < 50)
        {
            digitalWrite(LED, HIGH);
            delay(3000);
            digitalWrite(LED, LOW);
            ck = true;
            break;
        }

        if (time_check > 2)
        {
            ck = false;
            break;
        }

        delay(1000);
        time_check++;
    }

    return ck;
}

// -----------------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------------

App01::App01()
{
    // code
}

// setup
void App01::Setup()
{
    // init serial baud 115200
    Serial.begin(115200);

    // init SPIFFS
    SPIFFS.begin();
    // init pin mode
    pinMode(LED, OUTPUT);

    bool checkM = optionMODE();

    if (!checkM)
    {
        Serial.println("option mode online");
        ssid = ssid_on;
        password = password_on;
        serverName = serverName_on;
        mqttServer = mqttServer_on;
    }
    else
    {
        Serial.println("option mode offline");
        ssid = ssid_off;
        password = password_off;
        serverName = serverName_off;
        mqttServer = mqttServer_off;
    }

    Serial.println(ssid);
    Serial.println(password);

    // connectWiFi();
    connect2nearestAP();
    // get list devices
    device_HTTP_GET();
    // Can only be called once
    BLEDevice::init("BLE");
    // delay
    delay(100);
}

// run in loop
void App01::Loop()
{

    Serial.println();

    ScanBeacons();

    while (WiFi.status() != WL_CONNECTED)
    {
        ESP.restart();
    }

    while (!clientMQTT.connected())
    {
        connectMQTT();
    }

    if (WiFi.status() == WL_CONNECTED && clientMQTT.connected())
    {
        unsigned long timestamp = RTC.getEpoch();

        // char char_payload[u16t_MQTT_PACKET_SIZE];
        char *myArray = (char *)calloc(u16t_MQTT_PACKET_SIZE, sizeof(char));

        char_payloadJson(timestamp, myArray);

        // DEBUG_2("char_payload > \n", myArray);

        // String dataJson = payloadJson_01(timestamp);
        // DEBUG_2("dataJson.c_str()", dataJson.c_str());

        // size_t byteCount_dataJson = strlen(dataJson.c_str());
        // DEBUG_2("byteCount dataJson : ", byteCount_dataJson);

        // String encrypt_data = encrypt(dataJson, "VCC123");
        // size_t byteCount_encrypt_data = strlen(encrypt_data.c_str());
        // DEBUG_2("byteCount encrypt_data : ", byteCount_encrypt_data);

        // DEBUG_2("encrypt_data", decrypt(encrypt_data, "VCC123"));
        // Serial.print("MQTT state: ");
        // Serial.println(clientMQTT.state());

        bool resultMQTT = clientMQTT.publish(c_TOPIC, myArray);
        Serial.print("MQTT Result: ");
        Serial.println(resultMQTT);

        if (resultMQTT == 1)
        {
            digitalWrite(LED, HIGH);
        }

        free(myArray);
    }

    clientMQTT.loop();
    bufferIndex = 0;

    Serial.println("[APP] Free_heap_size: " + String(esp_get_free_heap_size()) + " bytes");

    delay(100);
    digitalWrite(LED, LOW);
}