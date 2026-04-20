#include "display_manager.h"

void setup() {
    Serial.begin(115200);
    displayInit();
    displayFill(0x0517);   // blu notte
    Serial.println("Display init OK");
}

void loop() {}
