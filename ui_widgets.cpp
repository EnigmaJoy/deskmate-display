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
