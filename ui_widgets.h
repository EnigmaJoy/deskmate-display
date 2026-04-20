#pragma once
#include "display_manager.h"
#include "data_store.h"
#include "wifi_manager.h"

// ── Palette Midnight Blue (RGB565) ───────────────────────────────────────
#define COL_BG      0x0517u   // #0a0e1a  sfondo
#define COL_WIDGET  0x0CA9u   // #0d1533  sfondo card
#define COL_BORDER  0x0F2Fu   // #1e3a5f  bordi
#define COL_TEXT    0xEF7Eu   // #e0f2fe  testo principale
#define COL_SUBTLE  0x9492u   // #94a3b8  testo secondario
#define COL_BLUE    0x3052u   // #60a5fa  accent ora/WiFi
#define COL_YELLOW  0xFDC4u   // #fbbf24  accent meteo
#define COL_PURPLE  0xA45Fu   // #a78bfa  accent crypto
#define COL_GREEN   0x1B35u   // #34d399  accent sensori / positivo
#define COL_RED     0xF811u   // #f87171  errori / negativo

// ── Coordinate layout (pixel) ────────────────────────────────────────────
#define STATUS_H   30
#define HALF_W    400
#define TOP_H     250
#define BOT_Y     280
#define BOT_H     200

// ── Dashboard ────────────────────────────────────────────────────────────
void drawStatusBar(const WeatherData& w, const CryptoData& c, const SensorData& s);
void drawWeather(const WeatherData& d);
void drawCrypto(const CryptoData& d);
void drawSensors(const SensorData& d);

// ── Viste dettaglio ──────────────────────────────────────────────────────
void drawWeatherDetail(const WeatherData& d);
void drawCryptoDetail(const CryptoData& d);
void drawSensorDetail(const SensorData& d);
