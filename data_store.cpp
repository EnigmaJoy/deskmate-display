#include "data_store.h"

WeatherData       g_weather;
CryptoData        g_crypto;
SensorData        g_sensor;
SemaphoreHandle_t weather_mutex = nullptr;
SemaphoreHandle_t crypto_mutex  = nullptr;

void dataStoreInit() {
    weather_mutex = xSemaphoreCreateMutex();
    crypto_mutex  = xSemaphoreCreateMutex();
}
