#include "sensor_manager.h"
#include <DHT.h>

#define DHT_PIN  17
#define DHT_TYPE DHT11

static DHT           dht(DHT_PIN, DHT_TYPE);
static unsigned long lastRead = 0;

void sensorInit() {
    dht.begin();
}

void sensorReadIfDue() {
    if (millis() - lastRead < 5000UL) return;
    lastRead = millis();

    float t = dht.readTemperature();
    float h = dht.readHumidity();

    // Fino a 3 tentativi se errore
    for (int i = 0; i < 3 && (isnan(t) || isnan(h)); i++) {
        delay(500);
        t = dht.readTemperature();
        h = dht.readHumidity();
    }

    if (isnan(t) || isnan(h)) {
        g_sensor.valid = false;
        Serial.println("[DHT11] Read failed after 3 retries");
        return;
    }

    // g_sensor: no mutex — written and read only on Core 1 (see data_store.h)
    g_sensor.temp_c       = t;
    g_sensor.humidity_pct = h;
    g_sensor.valid        = true;

    struct tm ti;
    getLocalTime(&ti);
    snprintf(g_sensor.updated_at, sizeof(g_sensor.updated_at),
             "%02d:%02d:%02d", ti.tm_hour, ti.tm_min, ti.tm_sec);

    Serial.printf("[DHT11] %.1f C  %.0f%%\n", t, h);
}
