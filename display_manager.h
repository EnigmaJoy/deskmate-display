#pragma once
#include <LovyanGFX.hpp>
#include <lgfx/v1/platforms/esp32s3/Panel_RGB.hpp>
#include <lgfx/v1/platforms/esp32s3/Bus_RGB.hpp>

class LGFX : public lgfx::LGFX_Device {
    lgfx::Bus_RGB     _bus_instance;
    lgfx::Panel_RGB   _panel_instance;
    lgfx::Light_PWM   _light_instance;
    lgfx::Touch_GT911 _touch_instance;
public:
    LGFX();
};

extern LGFX        lcd;
extern LGFX_Sprite canvas;

void displayInit();
void displayFill(uint32_t color);   // helper debug
void showSetupScreen();
void showConnectingScreen();
void showErrorScreen(const String& msg);
