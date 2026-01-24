#include "WifiManager.h"

void WifiManager::begin(const char* ssid, const char* password) {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  lastAttemptMs_ = millis();
}

void WifiManager::update() {
  if (WiFi.status() == WL_CONNECTED) {
    return;
  }
  unsigned long now = millis();
  if (now - lastAttemptMs_ > 10000) {
    WiFi.disconnect();
    WiFi.reconnect();
    lastAttemptMs_ = now;
  }
}

bool WifiManager::isConnected() const {
  return WiFi.status() == WL_CONNECTED;
}
