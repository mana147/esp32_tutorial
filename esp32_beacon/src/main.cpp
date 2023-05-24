#include <Arduino.h>
#include <WiFi.h>
#include <ESP.h>
#include <SPIFFS.h>

const char *ssid = "P301";
const char *password = "123456789";

// ------------------------------------------------------
// ---------------- function prototype ------------------
// ------------------------------------------------------

void connection_wifi();
int get_core_id();


// ------------------------------------------------------
// ---------------- function definition ------------------
// ------------------------------------------------------

void connection_wifi()
{
  Serial.println("Connecting");
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to WiFi network with IP Address: ");
  Serial.println(WiFi.localIP());
}


int get_core_id()
{
  Serial.print("setup() running on core ");
  Serial.println(xPortGetCoreID());
  return xPortGetCoreID();
}

//-------------------------------------------------------
//-------------------------------------------------------
//-------------------------------------------------------

void setup()
{
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  connection_wifi();
  get_core_id();
}

//-------------------------------------------------------
//-------------------------------------------------------
//-------------------------------------------------------

void loop()
{
}

//-------------------------------------------------------
//-------------------------------------------------------
//-------------------------------------------------------

// Sketch uses 643190 bytes (30%) of program storage space. Maximum is 2097152 bytes.
// Global variables use 37948 bytes (11%) of dynamic memory, leaving 289732 bytes for local variables. Maximum is 327680 bytes.