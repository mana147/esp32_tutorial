#include <Arduino.h>
#include <WiFi.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "esp_log.h"

// -----------------------------------------
// -----------------------------------------
// -----------------------------------------

// const char *ssid = "VCCorp";
// const char *password = "Vcc123**";
const char *ssid = "P301";
const char *password = "123456789";

// -----------------------------------------
// -----------------------------------------
// -----------------------------------------

void setup()
{
	Serial.begin(115200);
	while (!Serial)
	{
		delay(100);
	}
	log_i(" Begins ");

	WiFi.mode(WIFI_MODE_STA);

	WiFi.disconnect(true, true);
	WiFi.begin(ssid, password);

	while (WiFi.status() != WL_CONNECTED)
	{
		delay(500);
		log_i(".");
	}
}

// -----------------------------------------
// -----------------------------------------
// -----------------------------------------

void loop()
{
	Serial.println("Scan start");

	// WiFi.scanNetworks will return the number of networks found.
	int n = WiFi.scanNetworks();
	Serial.println("Scan done");
	if (n == 0)
	{
		Serial.println("no networks found");
	}
	else
	{
		Serial.print(n);
		Serial.println(" networks found");
		Serial.println("Nr | SSID                             | RSSI | CH | Encryption");
		for (int i = 0; i < n; ++i)
		{
			// Print SSID and RSSI for each network found
			Serial.printf("%2d", i + 1);
			Serial.print(" | ");
			Serial.printf("%-32.32s", WiFi.SSID(i).c_str());
			Serial.print(" | ");
			Serial.printf("%4d", WiFi.RSSI(i));
			Serial.print(" | ");
			Serial.printf("%2d", WiFi.channel(i));
			Serial.print(" | ");
			Serial.println(WiFi.BSSIDstr(i));
			Serial.print(" | ");

			byte bssid[6];

			switch (WiFi.encryptionType(i))
			{
			case WIFI_AUTH_OPEN:
				Serial.print("open");
				break;
			case WIFI_AUTH_WEP:
				Serial.print("WEP");
				break;
			case WIFI_AUTH_WPA_PSK:
				Serial.print("WPA");
				break;
			case WIFI_AUTH_WPA2_PSK:
				Serial.print("WPA2");
				break;
			case WIFI_AUTH_WPA_WPA2_PSK:
				Serial.print("WPA+WPA2");
				break;
			case WIFI_AUTH_WPA2_ENTERPRISE:
				Serial.print("WPA2-EAP");
				break;
			case WIFI_AUTH_WPA3_PSK:
				Serial.print("WPA3");
				break;
			case WIFI_AUTH_WPA2_WPA3_PSK:
				Serial.print("WPA2+WPA3");
				break;
			case WIFI_AUTH_WAPI_PSK:
				Serial.print("WAPI");
				break;
			default:
				Serial.print("unknown");
			}
			Serial.println();
			delay(10);
		}
	}
	Serial.println("");

	// Delete the scan result to free memory for code below.
	WiFi.scanDelete();

	// Wait a bit before scanning again.
	delay(5000);
}

// -----------------------------------------
// -----------------------------------------
// -----------------------------------------