#include "display_manager.h"
#include "wifi_manager.h"

enum class AppState { WIFI_SETUP, CONNECTING, DASHBOARD, ERROR };
static AppState state    = AppState::CONNECTING;
static String   errorMsg;

void onPortalActive() {
    state = AppState::WIFI_SETUP;
    showSetupScreen();
}

void setup() {
    Serial.begin(115200);
    displayInit();

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
}

void loop() {
    if (state != AppState::DASHBOARD) return;
    delay(33);
}
