#include "app_v02.h"
#include "app_v02.config.h"
#include <Arduino.h>

// -----------------------------------------------------------------------------------------

void xorEncrypt(char *data, const char *key)
{
    size_t dataLength = strlen(data);
    // size_t keyLength = strlen(key);
    for (size_t i = 0; i < dataLength; i++)
    {
        data[i] = data[i] ^ key[i];
    }
}

App02::App02()
{
    // code
}

void App02::Setup()
{
    Serial.begin(115200);
    const char *key = "vcc123";
    char data[155] = "{\"name\":\"esp_001_001\",\"list_beacon_data\": [{\"name\":\"dps1\",\"address\":\"20:22:07:01:01:d9\",\"distance\":\"  0.35\"}],\"time\":\"1689583735\",\"heap_size\":\"52276\"} ";

    xorEncrypt(data, key);

    int length = sizeof(data) / sizeof(char);
    for (size_t i = 0; i < length; i++)
    {
        DEBUG_2("str   ", data[i]);
    }

    Serial.print(data);
}

void App02::Loop()
{
    delay(100);
}

// -----------------------------------------------------------------------------------------