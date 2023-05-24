
#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#define MQTT_MAX_PACKET_SIZE 1844
#include <PubSubClient.h>

const char *ssid = "ESP8266";
const char *password = "0123456789";

String serverName = "http://192.168.4.1/";

unsigned long lastTime = 0;
// Timer set to 10 minutes (600000)
// unsigned long timerDelay = 600000;
// Set timer to 5 seconds (5000)
unsigned long timerDelay = 5000;

/* MQTT credentials and connection */
const char *mqttServer = "192.168.4.3";
const int mqttPort = 1883;
const char *mqttUser = "";
const char *mqttPassword = "";
uint8_t message_char_buffer[MQTT_MAX_PACKET_SIZE];

WiFiClient espClient;
PubSubClient client(espClient);

void connectWiFi()
{
	WiFi.begin(ssid, password); 
	while (WiFi.status() != WL_CONNECTED)
	{
		delay(500);
		Serial.println("Connecting to WiFi..");
	}
	Serial.println("Connected to the WiFi network");

	Serial.println("");
	Serial.print("Connected to WiFi network with IP Address: ");
	Serial.println(WiFi.localIP());
}

void connectMQTT()
{
	client.setServer(mqttServer, mqttPort);
	Serial.println("Connecting to MQTT...");
	if (client.connect("ESP32Client", mqttUser, mqttPassword))
	{
		Serial.println("connected");
	}
	else
	{
		Serial.print("failed with state ");
		Serial.print(client.state());
		delay(2000);
	}
}

void setup()
{
	Serial.begin(115200);
	connectWiFi();
}

void loop()
{
	boolean result;
	// Reconnect WiFi if not connected
	while (WiFi.status() != WL_CONNECTED)
	{
		connectWiFi();
	}

	// Reconnect to MQTT if not connected
	while (!client.connected())
	{
		connectMQTT();
	}
	client.loop();

	// SenML begins
	String payloadString = "{\"e\": esp/123/123 ";
	payloadString += "\"}";

	// Print and publish payload
	Serial.print("MAX len: ");
	Serial.println(MQTT_MAX_PACKET_SIZE);

	Serial.print("Payload length: ");
	Serial.println(payloadString.length());
	Serial.println(payloadString);

	payloadString.getBytes(message_char_buffer, payloadString.length() + 1);
	result = client.publish("/beacons/office", message_char_buffer, payloadString.length(), false);
	Serial.print("PUB Result: ");
	Serial.println(result);

	// Start over the scan loop
	//  Add delay to slow down publishing frequency if needed.
	delay(1000);
}