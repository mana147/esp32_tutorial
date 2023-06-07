#include <Arduino.h>
#include <Arduino_JSON.h>
#include <HTTPClient.h>
#include <WiFi.h>
#include <freertos/FreeRTOS.h>
#include <esp_task_wdt.h>

#define num_device 200

// const char *ssid = "P301";
// const char *password = "123456789";
const char *ssid = "VCCorp";
const char *password = "Vcc123**";

String array_name_device[num_device];

// ------------------------------------------------------------------------------

class TimestampConverter
{
public:
	static void convertTimestamp(unsigned long timestamp, int &year, int &month, int &day, int &hour, int &minute, int &second)
	{
		time_t rawTime = timestamp;
		struct tm *timeInfo;

		timeInfo = localtime(&rawTime);

		year = timeInfo->tm_year + 1900;
		month = timeInfo->tm_mon + 1;
		day = timeInfo->tm_mday;

		hour = timeInfo->tm_hour;
		minute = timeInfo->tm_min;
		second = timeInfo->tm_sec;
	}
};

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

// ------------------------------------------------------------------------------
void connectWiFi()
{
	WiFi.disconnect();
	WiFi.begin(ssid, password);
	while (WiFi.status() != WL_CONNECTED)
	{
		delay(500);
		Serial.println("Connecting to WiFi..");
	}
	Serial.println("Connected to the WiFi network");
}
// ------------------------------------------------------------------------------
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

			Serial.print("current_time");
			Serial.println(current_time);

			int year, month, day, hour, minute, second;

			TimestampConverter::convertTimestamp(current_time, year, month, day, hour, minute, second);

			// In các thành phần ngày, tháng, năm, giờ, phút và giây
			Serial.print("Date: ");
			Serial.print(day);
			Serial.print("/");
			Serial.print(month);
			Serial.print("/");
			Serial.println(year);

			Serial.print("Time: ");
			Serial.print(hour);
			Serial.print(":");
			Serial.print(minute);
			Serial.print(":");
			Serial.println(second);

			vTaskDelay(pdMS_TO_TICKS(100));
		}

		if (myObject.hasOwnProperty("data"))
		{
			Serial.print("myObject[\"data\"] = ");
			Serial.println(myObject["data"]);

			int leng_data = myObject["data"].length();

			for (int i = 0; i <= leng_data; i++)
			{
				String name = myObject["data"][i]["name"];
				array_name_device[i] = name;

				vTaskDelay(pdMS_TO_TICKS(100));
			}
		}
	}

	// Free resources
	http.end();
}

// ------------------------------------------------------------------------------

void main_task(void *param)
{
	connectWiFi();

	device_HTTP_GET();

	while (true)
	{
		// Serial.println(xPortGetCoreID());
		vTaskDelay(pdMS_TO_TICKS(1000));
	}
}

// ----------------------------------------------------------
// ----------------------------------------------------------
// ----------------------------------------------------------

void setup()
{
	Serial.begin(115200);
	delay(500);

	// make sure we don't get killed for our long running tasks
	esp_task_wdt_init(10, false);

	xTaskCreate(main_task, "Main", 4096, NULL, 0, NULL);

	// xTaskCreatePinnedToCore(main_task, "Main", 4096, NULL, 0, NUL);
	// put your setup code here, to run once:
	// int result = myFunction(2, 3);
	// Serial.println(result);

	// create our application
}

// ----------------------------------------------------------
// ----------------------------------------------------------
// ----------------------------------------------------------

void loop()
{
	vTaskDelay(1000);
}
// ----------------------------------------------------------
// ----------------------------------------------------------
// ----------------------------------------------------------
