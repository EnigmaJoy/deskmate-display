# Deskmate Display — Design Spec

**Data:** 2026-04-21
**Hardware:** CrowPanel ESP32 5" (SKU: DIS07050H), ESP32-S3-WROOM-1-N4R8, 800×480 TFT-LCD

---

## Overview

Dashboard nativa disegnata direttamente sul display fisico con LovyanGFX — niente browser, niente React. Mostra meteo, crypto, sensori locali e status bar. Supporta touch (tap per vista dettaglio, long-press per refresh forzato).

---

## Pin hardware

### Display RGB
| Segnale | GPIO |
|---|---|
| D0–D15 | 8, 3, 46, 9, 1, 5, 6, 7, 15, 16, 4, 45, 48, 47, 21, 14 |
| HSYNC | 39 |
| VSYNC | 41 |
| PCLK | 0 |
| HENABLE | 40 |
| Backlight | 2 |

### SD Card (SPI)
| Segnale | GPIO |
|---|---|
| CS | 10 |
| MOSI | 11 |
| MISO | 13 |
| SCK | 12 |

### Periferiche
| Periferica | GPIO |
|---|---|
| Touch SDA (GT911) | 19 |
| Touch SCL (GT911) | 20 |
| DHT11 | **17** |

---

## Architettura: Macchina a stati (FSM)

```
WIFI_SETUP → CONNECTING → DASHBOARD
                  ↓             ↓
                ERROR  ←────────┘
```

| Stato | Trigger | Comportamento |
|---|---|---|
| `WIFI_SETUP` | Nessuna credenziale in NVS | Avvia AP "Deskmate-Setup" via WiFiManager, display mostra istruzioni |
| `CONNECTING` | Credenziali presenti | Tenta connessione WiFi, display mostra attesa |
| `DASHBOARD` | WiFi connesso | Avvia fetch tasks FreeRTOS, entra nel render loop |
| `ERROR` | WiFi perso dopo 3 tentativi | Display mostra errore con causa |

---

## Struttura file

```
deskmate-display/
├── deskmate-display.ino     # Orchestrazione FSM, setup(), loop()
├── wifi_manager.h/.cpp      # Copiato da deskmate-firmware (invariato)
├── display_manager.h/.cpp   # LovyanGFX init + schermate setup/connecting/error
├── ui_widgets.h/.cpp        # Funzioni di disegno widget e viste dettaglio
├── data_store.h/.cpp        # Struct condivise + mutex FreeRTOS
├── fetch_weather.h/.cpp     # Task FreeRTOS Core 0: polling Open-Meteo
├── fetch_crypto.h/.cpp      # Task FreeRTOS Core 0: polling Binance
├── sensor_manager.h/.cpp    # Lettura DHT11 GPIO 17, Core 1
└── touch_manager.h/.cpp     # GT911 I2C, tap/long-press detection
```

### Responsabilità dei moduli

- **`deskmate-display.ino`** — FSM, transizioni di stato, orchestrazione. Zero logica di dominio.
- **`wifi_manager`** — `wifiManagerInit()`, `wifiConnect()`, `isConnected()`, `getSSID()`, `getIP()`. Identico a deskmate-firmware.
- **`display_manager`** — init LovyanGFX Bus_RGB/Panel_RGB, sprite double-buffer, schermate FSM (setup, connecting, error).
- **`ui_widgets`** — funzioni pure di disegno: `drawStatusBar()`, `drawWeather()`, `drawCrypto()`, `drawSensors()`, `drawWeatherDetail()`, `drawCryptoDetail()`, `drawSensorDetail()`.
- **`data_store`** — struct `WeatherData`, `CryptoData`, `SensorData` + `SemaphoreHandle_t weather_mutex`, `crypto_mutex`. Istanze globali esterne.
- **`fetch_weather`** — task pinnato su Core 0; `HTTPClient` → Open-Meteo API; parse JSON → `WeatherData`; `vTaskDelay(10 min)`.
- **`fetch_crypto`** — task pinnato su Core 0; `HTTPClient` → Binance REST API; parse JSON → `CryptoData`; `vTaskDelay(30 s)`.
- **`sensor_manager`** — legge DHT11 su GPIO 17 ogni 5 s; scrive `g_sensor` (stesso Core 1, no mutex necessario).
- **`touch_manager`** — polling GT911 ogni 50 ms; rileva TAP (<300 ms) e LONG_PRESS (>600 ms); emette `TouchEvent`.

---

## Layout UI — 800×480

### Vista principale (DASHBOARD)

```
┌─────────────────────────────────────────┐  y=0
│  STATUS BAR  (800×30)                   │  y=29
├────────────────────┬────────────────────┤  y=30
│                    │                    │
│   METEO            │   CRYPTO           │
│   (400×250)        │   (400×250)        │
│   x=0, y=30        │   x=400, y=30      │
│                    │                    │
├────────────────────┴────────────────────┤  y=280
│                                         │
│   SENSORI  (800×200)   x=0, y=280       │
│                                         │
└─────────────────────────────────────────┘  y=479
```

### Coordinate LovyanGFX

```cpp
// Layout constants
#define STATUS_BAR_X   0
#define STATUS_BAR_Y   0
#define STATUS_BAR_W   800
#define STATUS_BAR_H   30

#define WEATHER_X      0
#define WEATHER_Y      30
#define WEATHER_W      400
#define WEATHER_H      250

#define CRYPTO_X       400
#define CRYPTO_Y       30
#define CRYPTO_W       400
#define CRYPTO_H       250

#define SENSOR_X       0
#define SENSOR_Y       280
#define SENSOR_W       800
#define SENSOR_H       200
```

### Stile visivo: Midnight Blue

| Elemento | Colore |
|---|---|
| Sfondo | `#0a0e1a` |
| Sfondo widget | `#0d1533` (gradiente verso `#0f1f3d`) |
| Bordi | `#1e3a5f` |
| Testo principale | `#e0f2fe` |
| Testo secondario | `#94a3b8` |
| Accent meteo | `#fbbf24` |
| Accent crypto | `#a78bfa` |
| Accent sensori | `#34d399` |
| Accent status/ora | `#60a5fa` |
| Variazione positiva | `#34d399` |
| Variazione negativa | `#f87171` |

### Contenuto widget

**Status bar** — ora (HH:MM:SS), data, icona WiFi + SSID/IP

**Meteo** — temperatura corrente (grande), condizione, min/max, vento km/h. Fonte: Open-Meteo API.

**Crypto** — BTC, ETH, SOL con prezzo e variazione 24h (%). Fonte: Binance REST API.

**Sensori** — temperatura (°C) e umidità (%) da DHT11 GPIO 17.

---

## Viste dettaglio (full screen, 800×480)

Attivate con TAP sul widget corrispondente. Tap ovunque per tornare a DASHBOARD.

| Vista | Contenuto extra |
|---|---|
| `DETAIL_WEATHER` | Forecast orario (6 slot), umidità, vento dettagliato |
| `DETAIL_CRYPTO` | Prezzi più grandi, variazione 24h evidenziata per tutti e tre |
| `DETAIL_SENSOR` | Temperatura e umidità a caratteri grandi, timestamp ultima lettura |

---

## Data flow: Dual-core FreeRTOS

### Core 0 — Network

```
fetch_weather task  →  HTTPClient → open-meteo.com
                    →  parse JSON
                    →  xSemaphoreTake(weather_mutex)
                    →  g_weather = nuovi_dati
                    →  xSemaphoreGive(weather_mutex)
                    →  vTaskDelay(10 min)

fetch_crypto task   →  HTTPClient → api.binance.com
                    →  parse JSON (BTCUSDT, ETHUSDT, SOLUSDT)
                    →  xSemaphoreTake(crypto_mutex)
                    →  g_crypto = nuovi_dati
                    →  xSemaphoreGive(crypto_mutex)
                    →  vTaskDelay(30 s)
```

### Core 1 — Display

```
loop() ogni ~33ms (30 fps):
  1. touch_manager.poll()        → TouchEvent
  2. aggiorna view_state         → DASHBOARD / DETAIL_*
  3. sensor_manager.readIfDue()  → g_sensor (ogni 5 s)
  4. xSemaphoreTake(weather_mutex, 0) → copia locale → Give
  5. xSemaphoreTake(crypto_mutex, 0)  → copia locale → Give
  6. ui_widgets.draw*(sprite, dati_locali)
  7. sprite.pushSprite(0, 0)
```

### Touch events

```cpp
enum class TouchEvent  { NONE, TAP, LONG_PRESS };
enum class ViewState   { DASHBOARD, DETAIL_WEATHER, DETAIL_CRYPTO, DETAIL_SENSOR };

// Logica:
// TAP su zona meteo   → DETAIL_WEATHER
// TAP su zona crypto  → DETAIL_CRYPTO
// TAP su zona sensori → DETAIL_SENSOR
// TAP in DETAIL_*     → DASHBOARD
// LONG_PRESS ovunque  → refresh forzato (notifica entrambi i task via flag)
```

### Frequenze di aggiornamento

| Sorgente | Frequenza normale | Trigger manuale |
|---|---|---|
| Open-Meteo | 10 minuti | LONG_PRESS |
| Binance | 30 secondi | LONG_PRESS |
| DHT11 | 5 secondi | — |
| Clock status bar | ogni secondo | — |

---

## Data store

```cpp
struct WeatherData {
    float temp, temp_min, temp_max, wind_kmh;
    float humidity_pct;                // per vista dettaglio
    char  condition[32];               // es. "Nuvoloso"
    char  updated_at[6];               // "14:22"
    bool  valid = false;
};

struct CryptoData {
    float btc, eth, sol;
    float btc_pct, eth_pct, sol_pct;  // variazione 24h %
    char  updated_at[6];
    bool  valid = false;
};

struct SensorData {
    float temp_c, humidity_pct;
    char  updated_at[9];               // "14:32:05"
    bool  valid = false;
};

// Globali (data_store.cpp)
extern WeatherData      g_weather;
extern CryptoData       g_crypto;
extern SensorData       g_sensor;       // no mutex (Core 1 only)
extern SemaphoreHandle_t weather_mutex;
extern SemaphoreHandle_t crypto_mutex;
```

---

## Gestione errori

| Scenario | Comportamento |
|---|---|
| HTTP fallisce | Widget mostra ultimo valore valido + timestamp. Se mai ricevuto: mostra "---" in grigio. |
| DHT11 errore lettura | Ritenta 3 volte (intervallo 500 ms). Se fallisce ancora: mostra "Err" nel widget. Non crasha. |
| WiFi disconnesso | Status bar mostra icona WiFi rossa. Fetch tasks si mettono in pausa (`isConnected()` check). Display continua a girare normalmente. |
| JSON malformato | Parse fallisce silenziosamente, `valid` rimane false, il vecchio dato resta sul display. |
| Mutex timeout | `xSemaphoreTake(..., 0)` non-bloccante: se il mutex è occupato, il render loop usa i dati del frame precedente. |

---

## Librerie

| Libreria | Uso |
|---|---|
| `LovyanGFX` | Driver display Bus_RGB/Panel_RGB, sprite double-buffer |
| `WiFiManager` by tzapu | Captive portal configurazione WiFi |
| `HTTPClient` (built-in ESP32) | Chiamate REST API su Core 0 |
| `ArduinoJson` | Parse JSON meteo e crypto |
| `DHT sensor library` by Adafruit | Lettura DHT11 GPIO 17 |
| `FreeRTOS` (built-in ESP32) | Task dual-core, mutex |
