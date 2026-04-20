# Deskmate Display Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Build a native ESP32-S3 dashboard on CrowPanel 5" (800×480) that shows weather, crypto, and DHT11 sensor data using LovyanGFX, with dual-core FreeRTOS for non-blocking HTTP fetching.

**Architecture:** Dual-core FreeRTOS — Core 0 runs two HTTP fetch tasks (Open-Meteo every 10 min, Binance every 30 s); Core 1 runs the render loop at ~30 fps, DHT11 polling, and GT911 touch detection. Shared data is exchanged via mutex-protected global structs. The full 800×480 frame is drawn into a PSRAM sprite and pushed atomically to the RGB display via LovyanGFX.

**Tech Stack:** Arduino IDE, LovyanGFX (Bus_RGB/Panel_RGB), WiFiManager (tzapu), HTTPClient, ArduinoJson v7, DHT sensor library (Adafruit), FreeRTOS (built-in ESP32)

---

## File Map

| File | Responsabilità |
|---|---|
| `deskmate-display.ino` | FSM orchestration, `setup()`, `loop()` |
| `wifi_manager.h/.cpp` | Captive portal + WiFi connection (copiato da deskmate-firmware) |
| `display_manager.h/.cpp` | LGFX class, sprite PSRAM, schermate FSM (setup/connecting/error) |
| `data_store.h/.cpp` | Struct condivise + mutex FreeRTOS |
| `sensor_manager.h/.cpp` | DHT11 GPIO 17, legge ogni 5 s, scrive `g_sensor` |
| `touch_manager.h/.cpp` | GT911 via LovyanGFX, emette TAP / LONG_PRESS |
| `fetch_weather.h/.cpp` | Task FreeRTOS Core 0: Open-Meteo → `g_weather` |
| `fetch_crypto.h/.cpp` | Task FreeRTOS Core 0: Binance → `g_crypto` |
| `ui_widgets.h/.cpp` | Tutte le funzioni draw: dashboard + viste dettaglio |

---

## Task 1: LovyanGFX display init

**Files:**
- Create: `display_manager.h`
- Create: `display_manager.cpp`
- Modify: `deskmate-display.ino`

- [ ] **Step 1: Crea `display_manager.h`**

```cpp
#pragma once
#include <LovyanGFX.hpp>

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
```

- [ ] **Step 2: Crea `display_manager.cpp`**

```cpp
#include "display_manager.h"

LGFX        lcd;
LGFX_Sprite canvas(&lcd);

LGFX::LGFX() {
    // ── Bus RGB ──────────────────────────────────────────────────────────
    {
        auto cfg = _bus_instance.config();
        cfg.freq_write    = 14000000;
        cfg.pin_d0  =  8;  cfg.pin_d1  =  3;  cfg.pin_d2  = 46;
        cfg.pin_d3  =  9;  cfg.pin_d4  =  1;  cfg.pin_d5  =  5;
        cfg.pin_d6  =  6;  cfg.pin_d7  =  7;  cfg.pin_d8  = 15;
        cfg.pin_d9  = 16;  cfg.pin_d10 =  4;  cfg.pin_d11 = 45;
        cfg.pin_d12 = 48;  cfg.pin_d13 = 47;  cfg.pin_d14 = 21;
        cfg.pin_d15 = 14;
        cfg.pin_henable = 40;
        cfg.pin_vsync   = 41;
        cfg.pin_hsync   = 39;
        cfg.pin_pclk    =  0;
        cfg.hsync_polarity    = 0;
        cfg.hsync_front_porch = 8;
        cfg.hsync_pulse_width = 4;
        cfg.hsync_back_porch  = 43;
        cfg.vsync_polarity    = 0;
        cfg.vsync_front_porch = 8;
        cfg.vsync_pulse_width = 4;
        cfg.vsync_back_porch  = 12;
        cfg.pclk_idle_high    = false;
        _bus_instance.config(cfg);
    }
    // ── Panel ────────────────────────────────────────────────────────────
    {
        auto cfg = _panel_instance.config();
        cfg.memory_width  = 800;
        cfg.memory_height = 480;
        cfg.panel_width   = 800;
        cfg.panel_height  = 480;
        cfg.offset_x      = 0;
        cfg.offset_y      = 0;
        _panel_instance.config(cfg);
    }
    // ── Backlight ────────────────────────────────────────────────────────
    {
        auto cfg = _light_instance.config();
        cfg.pin_bl      = 2;
        cfg.invert      = false;
        cfg.freq        = 44100;
        cfg.pwm_channel = 7;
        _light_instance.config(cfg);
    }
    // ── Touch GT911 ──────────────────────────────────────────────────────
    {
        auto cfg = _touch_instance.config();
        cfg.x_min = 0;   cfg.x_max = 799;
        cfg.y_min = 0;   cfg.y_max = 479;
        cfg.pin_int         = -1;
        cfg.bus_shared      = false;
        cfg.offset_rotation = 0;
        cfg.i2c_port = 0;
        cfg.i2c_addr = 0x5D;
        cfg.pin_sda  = 19;
        cfg.pin_scl  = 20;
        cfg.freq     = 400000;
        _touch_instance.config(cfg);
    }
    setPanel(&_panel_instance);
    _panel_instance.setLight(&_light_instance);
    _panel_instance.setBus(&_bus_instance);
    _panel_instance.setTouch(&_touch_instance);
}

void displayInit() {
    lcd.init();
    lcd.setRotation(0);
    lcd.setBrightness(200);
    canvas.setPsram(true);
    canvas.setColorDepth(16);
    canvas.createSprite(800, 480);
}

void displayFill(uint32_t color) {
    canvas.fillScreen(color);
    canvas.pushSprite(0, 0);
}

void showSetupScreen() {
    canvas.fillScreen(0x0517);
    canvas.setTextColor(0x3052);
    canvas.setTextSize(2);
    canvas.drawString("Connettiti a: Deskmate-Setup", 40, 180);
    canvas.setTextColor(0x9492);
    canvas.setTextSize(1);
    canvas.drawString("Poi apri 192.168.4.1 nel browser", 40, 220);
    canvas.pushSprite(0, 0);
}

void showConnectingScreen() {
    canvas.fillScreen(0x0517);
    canvas.setTextColor(0x3052);
    canvas.setTextSize(2);
    canvas.drawString("Connessione WiFi...", 40, 220);
    canvas.pushSprite(0, 0);
}

void showErrorScreen(const String& msg) {
    canvas.fillScreen(0x0517);
    canvas.setTextColor(0xF811);
    canvas.setTextSize(2);
    canvas.drawString("Errore", 40, 180);
    canvas.setTextColor(0x9492);
    canvas.setTextSize(1);
    canvas.drawString(msg, 40, 220);
    canvas.pushSprite(0, 0);
}
```

- [ ] **Step 3: `deskmate-display.ino` minimo per test**

```cpp
#include "display_manager.h"

void setup() {
    Serial.begin(115200);
    displayInit();
    displayFill(0x0517);   // blu notte
    Serial.println("Display init OK");
}

void loop() {}
```

- [ ] **Step 4: Compila e flasha — verifica**

Sketch → Upload. Atteso:
- Serial: `Display init OK`
- Display: schermo blu scuro uniforme, zero artefatti

- [ ] **Step 5: Commit**

```bash
git add display_manager.h display_manager.cpp deskmate-display.ino
git commit -m "feat: LovyanGFX display init with Bus_RGB for CrowPanel 5\""
```

---

## Task 2: WiFi manager + FSM scaffold

**Files:**
- Copy: `wifi_manager.h/.cpp` (da `../deskmate-firmware/`)
- Modify: `deskmate-display.ino`

- [ ] **Step 1: Copia i file WiFiManager**

```bash
cp ../deskmate-firmware/wifi_manager.h ./wifi_manager.h
cp ../deskmate-firmware/wifi_manager.cpp ./wifi_manager.cpp
```

- [ ] **Step 2: Aggiorna `deskmate-display.ino` con FSM completa**

```cpp
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
```

- [ ] **Step 3: Flasha + verifica**

Con credenziali WiFi già in NVS: display mostra "Connessione WiFi..." poi schermo vuoto.
Con rete nuova: captive portal attivo, display mostra "Connettiti a: Deskmate-Setup".
Serial: `Connected: <SSID>  IP: <IP>`

- [ ] **Step 4: Commit**

```bash
git add wifi_manager.h wifi_manager.cpp deskmate-display.ino
git commit -m "feat: WiFiManager + FSM states WIFI_SETUP/CONNECTING/DASHBOARD/ERROR"
```

---

## Task 3: data_store — struct condivise + mutex

**Files:**
- Create: `data_store.h`
- Create: `data_store.cpp`
- Modify: `deskmate-display.ino`

- [ ] **Step 1: Crea `data_store.h`**

```cpp
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
```

- [ ] **Step 2: Crea `data_store.cpp`**

```cpp
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
```

- [ ] **Step 3: Aggiungi a `setup()` in `deskmate-display.ino`**

```cpp
#include "data_store.h"
// Aggiunge prima di wifiManagerInit():
dataStoreInit();
```

- [ ] **Step 4: Verifica compilazione**

Sketch → Verify (senza flash). Deve compilare senza errori o warning.

- [ ] **Step 5: Commit**

```bash
git add data_store.h data_store.cpp deskmate-display.ino
git commit -m "feat: data_store structs WeatherData/CryptoData/SensorData with FreeRTOS mutexes"
```

---

## Task 4: sensor_manager — DHT11 GPIO 17

**Files:**
- Create: `sensor_manager.h`
- Create: `sensor_manager.cpp`
- Modify: `deskmate-display.ino`

- [ ] **Step 1: Crea `sensor_manager.h`**

```cpp
#pragma once
#include "data_store.h"

void sensorInit();
void sensorReadIfDue();   // chiama ogni loop(); legge ogni 5 s
```

- [ ] **Step 2: Crea `sensor_manager.cpp`**

```cpp
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

    g_sensor.temp_c       = t;
    g_sensor.humidity_pct = h;
    g_sensor.valid        = true;

    struct tm ti;
    getLocalTime(&ti);
    snprintf(g_sensor.updated_at, sizeof(g_sensor.updated_at),
             "%02d:%02d:%02d", ti.tm_hour, ti.tm_min, ti.tm_sec);

    Serial.printf("[DHT11] %.1f C  %.0f%%\n", t, h);
}
```

- [ ] **Step 3: Aggiungi a `deskmate-display.ino`**

```cpp
#include "sensor_manager.h"
// In setup(), dopo dataStoreInit():
sensorInit();
// In loop(), prima di delay():
sensorReadIfDue();
```

- [ ] **Step 4: Flasha + verifica**

Serial monitor a 115200 baud. Ogni 5 s: `[DHT11] 24.5 C  58%`
Se appare `Read failed`: controlla cablaggio GPIO 17.

- [ ] **Step 5: Commit**

```bash
git add sensor_manager.h sensor_manager.cpp deskmate-display.ino
git commit -m "feat: sensor_manager reads DHT11 on GPIO 17 every 5s with retry"
```

---

## Task 5: touch_manager — GT911 TAP / LONG_PRESS

**Files:**
- Create: `touch_manager.h`
- Create: `touch_manager.cpp`
- Modify: `deskmate-display.ino`

- [ ] **Step 1: Crea `touch_manager.h`**

```cpp
#pragma once
#include "display_manager.h"

enum class TouchEvent { NONE, TAP, LONG_PRESS };

struct TouchPoint { int x = 0; int y = 0; };

void       touchInit();
TouchEvent touchPoll(TouchPoint& out);   // chiama ogni loop()
```

- [ ] **Step 2: Crea `touch_manager.cpp`**

```cpp
#include "touch_manager.h"

static bool       pressing   = false;
static uint32_t   pressStart = 0;
static TouchPoint pressOrigin;

void touchInit() {
    // GT911 inizializzato dentro LGFX — nessun init aggiuntivo
}

TouchEvent touchPoll(TouchPoint& out) {
    lgfx::touch_point_t tp[1];
    int count = lcd.getTouch(tp, 1);

    if (count > 0) {
        if (!pressing) {
            pressing     = true;
            pressStart   = millis();
            pressOrigin  = { (int)tp[0].x, (int)tp[0].y };
        }
        out = pressOrigin;
        return TouchEvent::NONE;
    } else {
        if (pressing) {
            pressing = false;
            uint32_t dur = millis() - pressStart;
            out = pressOrigin;
            if (dur >= 600) return TouchEvent::LONG_PRESS;
            if (dur >=  30) return TouchEvent::TAP;
        }
    }
    return TouchEvent::NONE;
}
```

- [ ] **Step 3: Aggiungi debug temporaneo a `loop()` in `deskmate-display.ino`**

```cpp
#include "touch_manager.h"
// In setup(), dopo sensorInit():
touchInit();
// In loop(), prima di delay():
{
    TouchPoint tp;
    TouchEvent ev = touchPoll(tp);
    if (ev == TouchEvent::TAP)        Serial.printf("TAP       x=%d y=%d\n", tp.x, tp.y);
    if (ev == TouchEvent::LONG_PRESS) Serial.printf("LONG_PRESS x=%d y=%d\n", tp.x, tp.y);
}
```

- [ ] **Step 4: Flasha + verifica**

Tocca brevemente: `TAP x=... y=...`
Tieni premuto 1 s: `LONG_PRESS x=... y=...`

- [ ] **Step 5: Rimuovi il blocco debug da `loop()`**

Elimina il blocco `{ TouchPoint tp; TouchEvent ev = ... }` aggiunto nel passo 3.

- [ ] **Step 6: Commit**

```bash
git add touch_manager.h touch_manager.cpp deskmate-display.ino
git commit -m "feat: touch_manager TAP/LONG_PRESS via GT911 on I2C SDA=19 SCL=20"
```

---

## Task 6: ui_widgets — dashboard con dati placeholder

**Files:**
- Create: `ui_widgets.h`
- Create: `ui_widgets.cpp`
- Modify: `deskmate-display.ino`

- [ ] **Step 1: Crea `ui_widgets.h`**

```cpp
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
```

- [ ] **Step 2: Crea `ui_widgets.cpp`**

```cpp
#include "ui_widgets.h"
#include <time.h>

// ── Helper card ──────────────────────────────────────────────────────────
static void drawCard(int x, int y, int w, int h) {
    canvas.fillRect(x, y, w, h, COL_WIDGET);
    canvas.drawRect(x, y, w, h, COL_BORDER);
}

// ── STATUS BAR ───────────────────────────────────────────────────────────
void drawStatusBar(const WeatherData&, const CryptoData&, const SensorData&) {
    canvas.fillRect(0, 0, 800, STATUS_H, COL_WIDGET);
    canvas.drawLine(0, STATUS_H - 1, 800, STATUS_H - 1, COL_BORDER);

    struct tm ti;
    getLocalTime(&ti);
    static const char* DAYS[]   = {"Dom","Lun","Mar","Mer","Gio","Ven","Sab"};
    static const char* MONTHS[] = {"Gen","Feb","Mar","Apr","Mag","Giu",
                                   "Lug","Ago","Set","Ott","Nov","Dic"};
    char buf[48];
    snprintf(buf, sizeof(buf), "%02d:%02d:%02d  %s %d %s %d",
             ti.tm_hour, ti.tm_min, ti.tm_sec,
             DAYS[ti.tm_wday], ti.tm_mday,
             MONTHS[ti.tm_mon], ti.tm_year + 1900);
    canvas.setTextColor(COL_BLUE);
    canvas.setTextSize(1);
    canvas.drawString(buf, 8, 9);

    bool wifiOk = isConnected();
    canvas.setTextColor(wifiOk ? COL_GREEN : COL_RED);
    canvas.drawString(wifiOk ? "WiFi OK" : "WiFi ERR", 730, 9);
}

// ── METEO ────────────────────────────────────────────────────────────────
void drawWeather(const WeatherData& d) {
    int x = 0, y = STATUS_H;
    drawCard(x + 4, y + 4, HALF_W - 8, TOP_H - 8);
    int cx = x + 12, cy = y + 12;

    canvas.setTextColor(COL_YELLOW);
    canvas.setTextSize(1);
    canvas.drawString("METEO", cx, cy);

    if (!d.valid) {
        canvas.setTextColor(COL_SUBTLE);
        canvas.setTextSize(2);
        canvas.drawString("---", cx, cy + 20);
        return;
    }

    char buf[24];
    snprintf(buf, sizeof(buf), "%.0f", d.temp);
    canvas.setTextColor(COL_TEXT);
    canvas.setTextSize(4);
    canvas.drawString(buf, cx, cy + 20);
    canvas.setTextSize(2);
    canvas.drawString("C", cx + 80, cy + 24);

    canvas.setTextColor(COL_SUBTLE);
    canvas.setTextSize(1);
    canvas.drawString(d.condition, cx, cy + 80);

    snprintf(buf, sizeof(buf), "%.0f / %.0f C", d.temp_min, d.temp_max);
    canvas.setTextColor(COL_YELLOW);
    canvas.drawString(buf, cx, cy + 100);

    snprintf(buf, sizeof(buf), "Vento %.0f km/h", d.wind_kmh);
    canvas.setTextColor(COL_BLUE);
    canvas.drawString(buf, cx, cy + 120);

    snprintf(buf, sizeof(buf), "Agg. %s", d.updated_at);
    canvas.setTextColor(COL_BORDER);
    canvas.drawString(buf, cx, cy + 210);
}

// ── CRYPTO ───────────────────────────────────────────────────────────────
void drawCrypto(const CryptoData& d) {
    int x = HALF_W, y = STATUS_H;
    drawCard(x + 4, y + 4, HALF_W - 8, TOP_H - 8);
    int cx = x + 12, cy = y + 12;

    canvas.setTextColor(COL_PURPLE);
    canvas.setTextSize(1);
    canvas.drawString("CRYPTO", cx, cy);

    if (!d.valid) {
        canvas.setTextColor(COL_SUBTLE);
        canvas.setTextSize(2);
        canvas.drawString("---", cx, cy + 20);
        return;
    }

    struct Coin { const char* name; float price; float pct; int row; };
    Coin coins[3] = {
        {"BTC", d.btc, d.btc_pct, cy + 24},
        {"ETH", d.eth, d.eth_pct, cy + 90},
        {"SOL", d.sol, d.sol_pct, cy + 156},
    };

    for (auto& c : coins) {
        char buf[24];
        canvas.setTextColor(COL_PURPLE);
        canvas.setTextSize(1);
        canvas.drawString(c.name, cx, c.row);

        snprintf(buf, sizeof(buf), "$%.0f", c.price);
        canvas.setTextColor(COL_TEXT);
        canvas.setTextSize(2);
        canvas.drawString(buf, cx + 40, c.row - 4);

        snprintf(buf, sizeof(buf), "%+.1f%%", c.pct);
        canvas.setTextColor(c.pct >= 0 ? COL_GREEN : COL_RED);
        canvas.setTextSize(1);
        canvas.drawString(buf, cx + 200, c.row);

        canvas.drawLine(cx, c.row + 18, x + HALF_W - 20, c.row + 18, COL_BORDER);
    }

    char buf[12];
    snprintf(buf, sizeof(buf), "Agg. %s", d.updated_at);
    canvas.setTextColor(COL_BORDER);
    canvas.drawString(buf, cx, cy + 210);
}

// ── SENSORI ──────────────────────────────────────────────────────────────
void drawSensors(const SensorData& d) {
    drawCard(4, BOT_Y + 4, 800 - 8, BOT_H - 8);
    int cx = 12, cy = BOT_Y + 12;

    canvas.setTextColor(COL_GREEN);
    canvas.setTextSize(1);
    canvas.drawString("SENSORI  DHT11 GPIO 17", cx, cy);

    if (!d.valid) {
        canvas.setTextColor(COL_RED);
        canvas.setTextSize(2);
        canvas.drawString("Err", cx, cy + 40);
        return;
    }

    char buf[20];
    canvas.setTextColor(COL_SUBTLE);
    canvas.setTextSize(1);
    canvas.drawString("Temperatura", cx, cy + 24);
    snprintf(buf, sizeof(buf), "%.1f C", d.temp_c);
    canvas.setTextColor(COL_TEXT);
    canvas.setTextSize(4);
    canvas.drawString(buf, cx, cy + 40);

    canvas.setTextColor(COL_SUBTLE);
    canvas.setTextSize(1);
    canvas.drawString("Umidita'", cx + 280, cy + 24);
    snprintf(buf, sizeof(buf), "%.0f%%", d.humidity_pct);
    canvas.setTextColor(COL_BLUE);
    canvas.setTextSize(4);
    canvas.drawString(buf, cx + 280, cy + 40);

    snprintf(buf, sizeof(buf), "Ultima lettura: %s", d.updated_at);
    canvas.setTextColor(COL_BORDER);
    canvas.setTextSize(1);
    canvas.drawString(buf, cx, cy + 140);
}

// ── WEATHER DETAIL ───────────────────────────────────────────────────────
void drawWeatherDetail(const WeatherData& d) {
    canvas.fillScreen(COL_BG);
    canvas.setTextColor(COL_SUBTLE);
    canvas.setTextSize(1);
    canvas.drawString("METEO DETTAGLIO  -  tocca per tornare", 12, 8);

    if (!d.valid) {
        canvas.setTextColor(COL_SUBTLE);
        canvas.setTextSize(2);
        canvas.drawString("Dati non disponibili", 40, 220);
        return;
    }

    char buf[48];
    snprintf(buf, sizeof(buf), "%.0f C", d.temp);
    canvas.setTextColor(COL_TEXT);
    canvas.setTextSize(5);
    canvas.drawString(buf, 40, 60);

    canvas.setTextColor(COL_SUBTLE);
    canvas.setTextSize(2);
    canvas.drawString(d.condition, 40, 150);

    snprintf(buf, sizeof(buf), "Min %.0f C   Max %.0f C", d.temp_min, d.temp_max);
    canvas.setTextColor(COL_YELLOW);
    canvas.setTextSize(1);
    canvas.drawString(buf, 40, 195);

    snprintf(buf, sizeof(buf), "Vento %.0f km/h   Umidita' %.0f%%", d.wind_kmh, d.humidity_pct);
    canvas.setTextColor(COL_BLUE);
    canvas.drawString(buf, 40, 215);

    snprintf(buf, sizeof(buf), "Aggiornato: %s", d.updated_at);
    canvas.setTextColor(COL_BORDER);
    canvas.drawString(buf, 40, 440);
}

// ── CRYPTO DETAIL ────────────────────────────────────────────────────────
void drawCryptoDetail(const CryptoData& d) {
    canvas.fillScreen(COL_BG);
    canvas.setTextColor(COL_SUBTLE);
    canvas.setTextSize(1);
    canvas.drawString("CRYPTO DETTAGLIO  -  tocca per tornare", 12, 8);

    if (!d.valid) {
        canvas.setTextColor(COL_SUBTLE);
        canvas.setTextSize(2);
        canvas.drawString("Dati non disponibili", 40, 220);
        return;
    }

    struct Coin { const char* name; float price; float pct; int y; };
    Coin coins[3] = {
        {"BTC", d.btc, d.btc_pct, 60},
        {"ETH", d.eth, d.eth_pct, 200},
        {"SOL", d.sol, d.sol_pct, 340},
    };

    for (auto& c : coins) {
        char buf[32];
        canvas.setTextColor(COL_PURPLE);
        canvas.setTextSize(2);
        canvas.drawString(c.name, 40, c.y);

        snprintf(buf, sizeof(buf), "$%.2f", c.price);
        canvas.setTextColor(COL_TEXT);
        canvas.setTextSize(3);
        canvas.drawString(buf, 120, c.y - 6);

        snprintf(buf, sizeof(buf), "%+.2f%%", c.pct);
        canvas.setTextColor(c.pct >= 0 ? COL_GREEN : COL_RED);
        canvas.setTextSize(2);
        canvas.drawString(buf, 560, c.y);

        canvas.drawLine(40, c.y + 36, 760, c.y + 36, COL_BORDER);
    }

    char buf[12];
    snprintf(buf, sizeof(buf), "Agg. %s", d.updated_at);
    canvas.setTextColor(COL_BORDER);
    canvas.setTextSize(1);
    canvas.drawString(buf, 40, 440);
}

// ── SENSOR DETAIL ────────────────────────────────────────────────────────
void drawSensorDetail(const SensorData& d) {
    canvas.fillScreen(COL_BG);
    canvas.setTextColor(COL_SUBTLE);
    canvas.setTextSize(1);
    canvas.drawString("SENSORI DETTAGLIO  -  tocca per tornare", 12, 8);

    if (!d.valid) {
        canvas.setTextColor(COL_RED);
        canvas.setTextSize(2);
        canvas.drawString("Err - DHT11 non risponde", 40, 220);
        return;
    }

    char buf[24];
    canvas.setTextColor(COL_SUBTLE);
    canvas.setTextSize(2);
    canvas.drawString("Temperatura", 80, 80);
    snprintf(buf, sizeof(buf), "%.1f C", d.temp_c);
    canvas.setTextColor(COL_TEXT);
    canvas.setTextSize(6);
    canvas.drawString(buf, 80, 130);

    canvas.setTextColor(COL_SUBTLE);
    canvas.setTextSize(2);
    canvas.drawString("Umidita'", 480, 80);
    snprintf(buf, sizeof(buf), "%.0f%%", d.humidity_pct);
    canvas.setTextColor(COL_BLUE);
    canvas.setTextSize(6);
    canvas.drawString(buf, 480, 130);

    snprintf(buf, sizeof(buf), "Ultima lettura: %s", d.updated_at);
    canvas.setTextColor(COL_BORDER);
    canvas.setTextSize(1);
    canvas.drawString(buf, 80, 420);
}
```

- [ ] **Step 3: Aggiungi render loop con ViewState a `deskmate-display.ino`**

```cpp
#include "ui_widgets.h"
#include "touch_manager.h"

enum class ViewState { DASHBOARD, DETAIL_WEATHER, DETAIL_CRYPTO, DETAIL_SENSOR };
static ViewState view = ViewState::DASHBOARD;

// In setup(), dopo wifiConnect() con successo:
touchInit();
configTzTime("CET-1CEST,M3.5.0,M10.5.0/3", "pool.ntp.org");

// loop() completo:
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
```

- [ ] **Step 4: Flasha + verifica**

Display: dashboard Midnight Blue con widget vuoti ("---"), status bar con ora NTP.
DHT11: dopo 5 s il widget sensori si popola.
Touch: tap su widget meteo → vista dettaglio. Tap di nuovo → torna dashboard.

- [ ] **Step 5: Commit**

```bash
git add ui_widgets.h ui_widgets.cpp deskmate-display.ino
git commit -m "feat: ui_widgets dashboard and detail views, render loop with ViewState"
```

---

## Task 7: fetch_weather — Open-Meteo API, Core 0

**Files:**
- Create: `fetch_weather.h`
- Create: `fetch_weather.cpp`
- Modify: `deskmate-display.ino`

Imposta `LAT` e `LON` con le coordinate della tua città (default: Milano).

- [ ] **Step 1: Crea `fetch_weather.h`**

```cpp
#pragma once
#include "data_store.h"

void fetchWeatherStart();   // avvia task FreeRTOS Core 0
void fetchWeatherNow();     // forza fetch immediato
```

- [ ] **Step 2: Crea `fetch_weather.cpp`**

```cpp
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
```

- [ ] **Step 3: Aggiungi a `setup()` in `deskmate-display.ino`**

```cpp
#include "fetch_weather.h"
// Dopo configTzTime():
fetchWeatherStart();
```

- [ ] **Step 4: Flasha + verifica**

Serial entro 10 s dalla connessione: `[Weather] 22.1 C  Nuvoloso`
Display: widget meteo si popola con dati reali.

- [ ] **Step 5: Commit**

```bash
git add fetch_weather.h fetch_weather.cpp deskmate-display.ino
git commit -m "feat: fetch_weather task Core 0 polling Open-Meteo every 10 min"
```

---

## Task 8: fetch_crypto — Binance API, Core 0

**Files:**
- Create: `fetch_crypto.h`
- Create: `fetch_crypto.cpp`
- Modify: `deskmate-display.ino`

- [ ] **Step 1: Crea `fetch_crypto.h`**

```cpp
#pragma once
#include "data_store.h"

void fetchCryptoStart();
void fetchCryptoNow();
```

- [ ] **Step 2: Crea `fetch_crypto.cpp`**

```cpp
#include "fetch_crypto.h"
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <WiFiClientSecure.h>
#include <WiFi.h>

static bool fetchSymbol(const char* url, float& price, float& pct) {
    WiFiClientSecure client;
    client.setInsecure();
    HTTPClient http;
    http.begin(client, url);
    http.setTimeout(10000);
    int code = http.GET();
    if (code != 200) {
        Serial.printf("[Crypto] HTTP %d  %s\n", code, url);
        http.end();
        return false;
    }
    JsonDocument doc;
    if (deserializeJson(doc, http.getStream())) {
        http.end();
        return false;
    }
    http.end();
    price = atof(doc["lastPrice"]          | "0");
    pct   = atof(doc["priceChangePercent"] | "0");
    return true;
}

static bool doFetch() {
    if (!WiFi.isConnected()) return false;

    float btc = 0, eth = 0, sol = 0;
    float btc_p = 0, eth_p = 0, sol_p = 0;

    bool ok =
        fetchSymbol("https://api.binance.com/api/v3/ticker/24hr?symbol=BTCUSDT", btc, btc_p) &&
        fetchSymbol("https://api.binance.com/api/v3/ticker/24hr?symbol=ETHUSDT", eth, eth_p) &&
        fetchSymbol("https://api.binance.com/api/v3/ticker/24hr?symbol=SOLUSDT", sol, sol_p);

    if (!ok) return false;

    CryptoData cd;
    cd.btc = btc; cd.btc_pct = btc_p;
    cd.eth = eth; cd.eth_pct = eth_p;
    cd.sol = sol; cd.sol_pct = sol_p;
    cd.valid = true;

    struct tm ti; getLocalTime(&ti);
    snprintf(cd.updated_at, sizeof(cd.updated_at), "%02d:%02d", ti.tm_hour, ti.tm_min);

    xSemaphoreTake(crypto_mutex, portMAX_DELAY);
    g_crypto = cd;
    xSemaphoreGive(crypto_mutex);

    Serial.printf("[Crypto] BTC $%.0f  ETH $%.0f  SOL $%.0f\n", btc, eth, sol);
    return true;
}

static volatile bool forceNow = false;

static void cryptoTask(void*) {
    const TickType_t interval = pdMS_TO_TICKS(30UL * 1000);
    for (;;) {
        doFetch();
        TickType_t deadline = xTaskGetTickCount() + interval;
        while (xTaskGetTickCount() < deadline) {
            if (forceNow) { forceNow = false; break; }
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }
}

void fetchCryptoStart() {
    xTaskCreatePinnedToCore(cryptoTask, "crypto", 8192, nullptr, 1, nullptr, 0);
}

void fetchCryptoNow() { forceNow = true; }
```

- [ ] **Step 3: Aggiungi a `setup()` in `deskmate-display.ino`**

```cpp
#include "fetch_crypto.h"
// Dopo fetchWeatherStart():
fetchCryptoStart();
```

- [ ] **Step 4: Flasha + verifica**

Serial ogni 30 s: `[Crypto] BTC $94200  ETH $3180  SOL $182`
Display: widget crypto si popola con prezzi e variazioni % reali.

- [ ] **Step 5: Commit**

```bash
git add fetch_crypto.h fetch_crypto.cpp deskmate-display.ino
git commit -m "feat: fetch_crypto task Core 0 polling Binance every 30s"
```

---

## Task 9: Long-press → force refresh

**Files:**
- Modify: `deskmate-display.ino`

- [ ] **Step 1: Aggiungi gestione LONG_PRESS nel `loop()`**

Nel blocco touch, dopo il blocco `if (ev == TouchEvent::TAP)` aggiungi:

```cpp
} else if (ev == TouchEvent::LONG_PRESS) {
    fetchWeatherNow();
    fetchCryptoNow();
    Serial.println("[Touch] Long press - force refresh");
}
```

- [ ] **Step 2: Flasha + verifica**

Tieni premuto 1 s: Serial mostra `[Touch] Long press - force refresh`
Entro 15 s: nuovi valori `[Weather]` e `[Crypto]` appaiono in Serial e sul display.

- [ ] **Step 3: Verifica stabilità 10 minuti**

Lascia girare il device per 10 minuti. Verifica:
- Nessun crash / watchdog reset
- Crypto si aggiorna ogni 30 s (visible da Serial)
- Meteo rimane stabile, si aggiornerà al prossimo ciclo da 10 min
- Clock sul display si aggiorna ogni secondo

- [ ] **Step 4: Commit**

```bash
git add deskmate-display.ino
git commit -m "feat: long-press force refresh weather and crypto, complete render loop"
```

---

## Stato finale di `deskmate-display.ino`

Per riferimento, il file completo al termine di tutti i task:

```cpp
#include "display_manager.h"
#include "wifi_manager.h"
#include "data_store.h"
#include "sensor_manager.h"
#include "touch_manager.h"
#include "fetch_weather.h"
#include "fetch_crypto.h"
#include "ui_widgets.h"

enum class AppState  { WIFI_SETUP, CONNECTING, DASHBOARD, ERROR };
enum class ViewState { DASHBOARD, DETAIL_WEATHER, DETAIL_CRYPTO, DETAIL_SENSOR };

static AppState  state = AppState::CONNECTING;
static ViewState view  = ViewState::DASHBOARD;
static String    errorMsg;

void onPortalActive() {
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
    fetchCryptoStart();
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
    } else if (ev == TouchEvent::LONG_PRESS) {
        fetchWeatherNow();
        fetchCryptoNow();
        Serial.println("[Touch] Long press - force refresh");
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
```
