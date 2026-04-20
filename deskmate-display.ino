#include "display_manager.h"
#include "wifi_manager.h"
#include "data_store.h"
#include "sensor_manager.h"
#include "touch_manager.h"
#include "ui_widgets.h"
#include "fetch_weather.h"

enum class AppState  { WIFI_SETUP, CONNECTING, DASHBOARD, ERROR };
enum class ViewState { DASHBOARD, DETAIL_WEATHER, DETAIL_CRYPTO, DETAIL_SENSOR };

static AppState  state = AppState::CONNECTING;
static ViewState view  = ViewState::DASHBOARD;
static String    errorMsg;

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
    configTzTime("CET-1CEST,M3.5.0,M10.5.0/3", "pool.ntp.org");
    fetchWeatherStart();
}

void loop() {
    if (state != AppState::DASHBOARD) return;

    sensorReadIfDue();

    TouchPoint tp;
    TouchEvent ev = touchPoll(tp);

    if (ev == TouchEvent::TAP) {
        if (view != ViewState::DASHBOARD) {
            view = ViewState::DASHBOARD;
        } else if (tp.y >= STATUS_H && tp.y < BOT_Y && tp.x < HALF_W) {
            view = ViewState::DETAIL_WEATHER;
        } else if (tp.y >= STATUS_H && tp.y < BOT_Y && tp.x >= HALF_W) {
            view = ViewState::DETAIL_CRYPTO;
        } else if (tp.y >= BOT_Y) {
            view = ViewState::DETAIL_SENSOR;
        }
    }

    WeatherData wd;
    CryptoData  cd;
    SensorData  sd = g_sensor;
    if (xSemaphoreTake(weather_mutex, 0) == pdTRUE) { wd = g_weather; xSemaphoreGive(weather_mutex); }
    if (xSemaphoreTake(crypto_mutex,  0) == pdTRUE) { cd = g_crypto;  xSemaphoreGive(crypto_mutex);  }

    canvas.fillScreen(COL_BG);
    switch (view) {
        case ViewState::DASHBOARD:
            drawStatusBar(wd, cd, sd);
            drawWeather(wd);
            drawCrypto(cd);
            drawSensors(sd);
            break;
        case ViewState::DETAIL_WEATHER: drawWeatherDetail(wd); break;
        case ViewState::DETAIL_CRYPTO:  drawCryptoDetail(cd);  break;
        case ViewState::DETAIL_SENSOR:  drawSensorDetail(sd);  break;
    }
    canvas.pushSprite(0, 0);
    delay(33);
}
