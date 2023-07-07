#ifndef CONFIG_H_
#define CONFIG_H_

#define MaxSecWiFi 30           // max time in s for WiFi setup, function will return WiFiErrorCode if WiFi was not successful.
#define WiFiErrorCode 1000      // see above
#define SaveDisconnectTime 1000 // Time im ms for save disconnection, needed to avoid that WiFi works only evey secon boot: https://github.com/espressif/arduino-esp32/issues/2501
#define WiFiTime 100            // Max time in s for successful WiFi connection.
#define WiFiOK 0                // WiFi connected
#define WiFiTimedOut 1          // WiFi connection timed out
#define WiFiNoSSID 2            // The SSID was not found during network scan
#define WiFiRSSI -75            // Min. required signal strength for a good WiFi cennection. If lower, new scan is initiated in checkWiFi()

#define num_device 100
#define MQTT_MAX_PACKET_SIZE_ESP32 1048

/* WiFi username and password */

// // Tính toán độ dài của chuỗi kết quả
// int resultLength = strlen("event1/client/") + nameESP.length();
// // Cấp phát bộ nhớ đủ cho chuỗi kết quả
// char *Toptic = new char[resultLength + 1];
// // Copy nội dung của chuỗi topic vào chuỗi kết quả
// strcpy(Toptic, "event1/client/");
// // Nối chuỗi name vào chuỗi kết quả
// strcat(Toptic, nameESP.c_str());
// // Giải phóng bộ nhớ đã cấp phát
// delete[] Toptic;

#define ESP_NAME "esp_001_006"
const char *c_TOPIC = "event1/client/esp_001_006";

const char *ssid = "VCCorp";
const char *password = "Vcc123**";
// const char *ssid = "vcc-event1";
// const char *password = "A12345678";
// const char *ssid = "P301";
// const char *password = "123456789";

// const char *serverName = "http://phub.bctoyz.com";
// const char *api_part_log = "/api/v1/map/log-devices";
const char *serverName = "10.0.10.250";
const char *api_part_log = "/api/v1/map/log-devices";

// const char *ntpServer = "pool.ntp.org"; // vn.pool.ntp.org
// const long gmtOffset_sec = 3600;
// const int daylightOffset_sec = 3600;

/* MQTT credentials and connection */
const char *mqttServer = "10.0.10.250";
const int mqttPort = 1883;
const char *mqttUser = "esp_001_001";
const char *mqttPassword = "esp_001_001";

// const char *mqttServer = "mqtt.eclipseprojects.io";
// const int mqttPort = 1883;
// const char *mqttUser = "esp_001_001";
// const char *mqttPassword = "esp_001_001";

#endif