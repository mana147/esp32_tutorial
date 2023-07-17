#include "until/main.config.h"
#include "until/until.cpp"
#include "app_v01/app_v01.h"
// #include "app_v02/app_v02.h"

App01 App;
// App02 App;

void setup()
{
	App.Setup();
}

void loop()
{
	App.Loop();
}