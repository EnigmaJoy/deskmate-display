#include "display_manager.h"
#include "wifi_manager.h"
#include "data_store.h"
#include "sensor_manager.h"
#include "touch_manager.h"

enum class AppState { WIFI_SETUP, CONNECTING, DASHBOARD, ERROR };
static AppState state    = AppState::CONNECTING;
static String   errorMsg;

void onPortalActive() {
    Serial.println("[WiFi] Portal active - connect to Deskmate-Setup");
    state = AppState::WIFI_SETUP;
    showSetupScreen();
}

void setup() {
    Serial.begin(115200);
    displayInit();
    dataStoreInit();
    sensorInit();

    wifiManagerInit(onPortalActive);

    showConnectingScreen();
    if (!wifiConnect()) {
        errorMsg = "WiFi connection failed";
        state = AppState::ERROR;
        showErrorScreen(errorMsg);
        return;
    }

    Serial.printf("Connected: %s  IP: %s\n", getSSID().c_str(), getIP().c_str());
    state = AppState::DASHBOARD;
    touchInit();
}

void loop() {
    if (state != AppState::DASHBOARD) return;

    {
        TouchPoint tp;
        TouchEvent ev = touchPoll(tp);
        if (ev == TouchEvent::TAP)        Serial.printf("TAP       x=%d y=%d\n", tp.x, tp.y);
        if (ev == TouchEvent::LONG_PRESS) Serial.printf("LONG_PRESS x=%d y=%d\n", tp.x, tp.y);
    }

    sensorReadIfDue();
    delay(33);
}
