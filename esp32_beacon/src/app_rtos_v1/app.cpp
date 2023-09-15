#include "app.h"
#include "app.config.h"
#include <Arduino.h> 

// -----------------------------------------------------------------------------------------

Appbase::Appbase()
{
    // code
}

void Appbase::Setup()
{
    Serial.begin(115200);
}

void Appbase::Loop()
{
    delay(100);
}

// -----------------------------------------------------------------------------------------