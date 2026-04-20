#pragma once
#include <Arduino.h>

void wifiManagerInit(void (*portalCallback)());
bool wifiConnect();
bool isConnected();
String getSSID();
String getIP();
