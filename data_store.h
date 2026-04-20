#pragma once
#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

struct WeatherData {
    float temp         = 0;
    float temp_min     = 0;
    float temp_max     = 0;
    float wind_kmh     = 0;
    float humidity_pct = 0;
    char  condition[32] = "---";
    char  updated_at[6] = "--:--";
    bool  valid         = false;
};

struct CryptoData {
    float btc     = 0;  float btc_pct = 0;
    float eth     = 0;  float eth_pct = 0;
    float sol     = 0;  float sol_pct = 0;
    char  updated_at[6] = "--:--";
    bool  valid   = false;
};

struct SensorData {
    float temp_c       = 0;
    float humidity_pct = 0;
    char  updated_at[9] = "--:--:--";
    bool  valid        = false;
};

// g_sensor è scritto e letto solo su Core 1 — no mutex
extern WeatherData        g_weather;
extern CryptoData         g_crypto;
extern SensorData         g_sensor;
extern SemaphoreHandle_t  weather_mutex;
extern SemaphoreHandle_t  crypto_mutex;

void dataStoreInit();
