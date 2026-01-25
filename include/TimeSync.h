#pragma once

#include <Arduino.h>

class TimeSync {
 public:
  void begin();
  void update();
  bool isSynced() const;
  uint64_t nowMs() const;

 private:
  bool synced_ = false;
  uint64_t bootEpochMs_ = 0;
  unsigned long lastSyncMs_ = 0;
};
