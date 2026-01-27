#include "TimeSync.h"

#include "Config.h"

#include <time.h>

void TimeSync::begin() {
  configTime(0, 0, "pool.ntp.org", "time.nist.gov");
  lastSyncMs_ = 0;
}

void TimeSync::update() {
  unsigned long now = millis();
  if (lastSyncMs_ != 0) {
    if (synced_ && now - lastSyncMs_ < Config::kNtpResyncMs) {
      return;
    }
    if (!synced_ && now - lastSyncMs_ < 30000) {
      return;
    }
  }

  struct tm timeinfo;
  if (getLocalTime(&timeinfo, 2000)) {
    time_t epoch = mktime(&timeinfo);
    bootEpochMs_ = static_cast<uint64_t>(epoch) * 1000ULL - now;
    synced_ = true;
  } else {
    synced_ = false;
  }
  lastSyncMs_ = now;
}

bool TimeSync::isSynced() const {
  return synced_;
}

uint64_t TimeSync::nowMs() const {
  if (!synced_) {
    return static_cast<uint64_t>(millis());
  }
  return bootEpochMs_ + static_cast<uint64_t>(millis());
}
