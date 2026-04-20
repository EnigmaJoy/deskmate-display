#include "wifi_manager.h"
#include <WiFi.h>
#include <WiFiManager.h>

static void (*s_portalCallback)() = nullptr;
static WiFiManager wm;

// IMPORTANTE: funzione BLOCCANTE fino a 180s se WiFi non è configurato.
// - Credenziali in NVS: ritorna in pochi secondi dopo connessione riuscita.
// - Nessuna credenziale: apre AP "Deskmate-Setup" e attende l'utente per max 180s.
// Il callback viene invocato quando il captive portal si apre.
void wifiManagerInit(void (*portalCallback)()) {
  s_portalCallback = portalCallback;

  wm.setAPCallback([](WiFiManager*) {
    if (s_portalCallback) s_portalCallback();
  });

  WiFi.disconnect(true);
  WiFi.mode(WIFI_STA);
  delay(100);

  wm.setConnectRetries(3);
  wm.setConnectTimeout(10);
  wm.setConfigPortalTimeout(180);

  wm.autoConnect("Deskmate-Setup");
}

bool wifiConnect() {
  for (int i = 0; i < 3; i++) {
    if (WiFi.status() == WL_CONNECTED) return true;
    WiFi.reconnect();
    unsigned long start = millis();
    while (millis() - start < 10000) {
      if (WiFi.status() == WL_CONNECTED) return true;
      delay(500);
    }
  }
  return false;
}

bool isConnected() {
  return WiFi.status() == WL_CONNECTED;
}

String getSSID() {
  return WiFi.SSID();
}

String getIP() {
  return WiFi.localIP().toString();
}
