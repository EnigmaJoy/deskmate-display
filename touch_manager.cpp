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
