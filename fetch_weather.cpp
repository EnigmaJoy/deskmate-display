#include "fetch_weather.h"
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <WiFi.h>

#define LAT "45.46"   // Milano — modifica con le tue coordinate
#define LON "9.19"

static const char* URL =
    "http://api.open-meteo.com/v1/forecast"
    "?latitude=" LAT "&longitude=" LON
    "&current=temperature_2m,relativehumidity_2m,weathercode,windspeed_10m"
    "&daily=temperature_2m_max,temperature_2m_min"
    "&timezone=auto&forecast_days=1";

static const char* wmoToStr(int code) {
    if (code == 0)        return "Soleggiato";
    if (code <= 2)        return "Poco nuvoloso";
    if (code == 3)        return "Nuvoloso";
    if (code <= 49)       return "Nebbia";
    if (code <= 55)       return "Pioggia leggera";
    if (code <= 65)       return "Pioggia";
    if (code <= 75)       return "Neve";
    if (code <= 82)       return "Pioggia forte";
    if (code <= 99)       return "Temporale";
    return "---";
}

static bool doFetch() {
    if (!WiFi.isConnected()) return false;

    HTTPClient http;
    http.begin(URL);
    http.setTimeout(10000);
    int code = http.GET();
    if (code != 200) {
        Serial.printf("[Weather] HTTP %d\n", code);
        http.end();
        return false;
    }

    JsonDocument doc;
    if (deserializeJson(doc, http.getStream())) {
        Serial.println("[Weather] JSON parse error");
        http.end();
        return false;
    }
    http.end();

    WeatherData wd;
    wd.temp         = doc["current"]["temperature_2m"]      | 0.0f;
    wd.humidity_pct = doc["current"]["relativehumidity_2m"] | 0.0f;
    wd.wind_kmh     = doc["current"]["windspeed_10m"]       | 0.0f;
    wd.temp_max     = doc["daily"]["temperature_2m_max"][0] | 0.0f;
    wd.temp_min     = doc["daily"]["temperature_2m_min"][0] | 0.0f;
    strlcpy(wd.condition, wmoToStr(doc["current"]["weathercode"] | 0), sizeof(wd.condition));
    wd.valid = true;

    struct tm ti; getLocalTime(&ti);
    snprintf(wd.updated_at, sizeof(wd.updated_at), "%02d:%02d", ti.tm_hour, ti.tm_min);

    xSemaphoreTake(weather_mutex, portMAX_DELAY);
    g_weather = wd;
    xSemaphoreGive(weather_mutex);

    Serial.printf("[Weather] %.1f C  %s\n", wd.temp, wd.condition);
    return true;
}

static volatile bool forceNow = false;

static void weatherTask(void*) {
    const TickType_t interval = pdMS_TO_TICKS(10UL * 60 * 1000);
    for (;;) {
        doFetch();
        TickType_t deadline = xTaskGetTickCount() + interval;
        while (xTaskGetTickCount() < deadline) {
            if (forceNow) { forceNow = false; break; }
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }
}

void fetchWeatherStart() {
    xTaskCreatePinnedToCore(weatherTask, "weather", 8192, nullptr, 1, nullptr, 0);
}

void fetchWeatherNow() { forceNow = true; }
