#include "app_base.h"
#include "app_base.config.h"
#include <Arduino.h>

// -----------------------------------------------------------------------------------------

AppBase::AppBase()
{
    // code
}

void AppBase::Setup()
{
    Serial.begin(115200);
}

void AppBase::Loop()
{
    delay(100);
}

// -----------------------------------------------------------------------------------------