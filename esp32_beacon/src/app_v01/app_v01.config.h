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

#define LED 2
#define PIN_NUMB_TOUCH 4

#define ESP_NAME "esp_001_001"
const char *c_TOPIC = "event1/client/esp/esp_001_001";
const char *c_PATH_FILE_TXT = "/data.txt";

char *mqttServer = nullptr;
char mqttServer_off[] = "10.0.10.250";
char mqttServer_on[] = "mqtt.bctoyz.com";

const int mqttPort = 1883;
const char *mqttUser = "esp_001_001";
const char *mqttPassword = "esp_001_001";

char *ssid = nullptr;
char *password = nullptr;

// char ssid_on[] = "VCCorp";
// char password_on[] = "Vcc123**";
char ssid_off[] = "vcc-event1";
char password_off[] = "A12345678";
char ssid_on[] = "HONGDO17_303";
char password_on[] = "123456789";

const char *c_recv_token = "Bearer 1|p02XlC16ykTHCnKRpQKM5iF6uijShnzqwtNE9h35";

char *serverName = nullptr;
char serverName_off[] = "10.0.10.250";
char serverName_on[] = "http://phub.bctoyz.com";

const char *api_part_log = "/api/v1/map/log-devices";
const char *api_v1_device = "/api/v1/device";
const char *api_system_time = "/api/v1/map/system-time";
const char *api_v1_device_beacon = "/api/v1/device/get-beacons";

/* MQTT credentials and connection */
// const char *mqttServer = "10.0.10.250";
// const int mqttPort = 1883;
// const char *mqttUser = "esp_001_001";
// const char *mqttPassword = "esp_001_001";

// const char *mqttServer = "mqtt.eclipseprojects.io";
// const int mqttPort = 1883;
// const char *mqttUser = "esp_001_001";
// const char *mqttPassword = "esp_001_001";

#endif