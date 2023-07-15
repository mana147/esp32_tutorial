#include "app_v02.h"
#include <Arduino.h>

#define DEBUG(n, x)    \
    Serial.print((n)); \
    Serial.println((x));
// -----------------------------------------------------------------------------------------

App02::App02()
{
    // code
}

void App02::Setup()
{
    Serial.begin(115200);

}

void App02::Loop()
{
}

// -----------------------------------------------------------------------------------------