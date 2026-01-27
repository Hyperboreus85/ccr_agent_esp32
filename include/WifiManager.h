#pragma once

#include <Arduino.h>
#include <WiFi.h>

class WifiManager {
 public:
  void begin(const char* ssid, const char* password);
  void update();
  bool isConnected() const;

 private:
  unsigned long lastAttemptMs_ = 0;
};
