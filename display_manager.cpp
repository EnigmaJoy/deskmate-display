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
        cfg.freq        = 10000;
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
    void* buf = canvas.createSprite(800, 480);
    if (!buf) {
        Serial.println("[Display] FATAL: sprite alloc failed - check PSRAM");
        while (true) { delay(1000); }
    }
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
