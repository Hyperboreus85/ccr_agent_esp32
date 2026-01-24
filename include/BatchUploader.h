#pragma once

#include "Config.h"
#include "StorageQueue.h"

#include <HTTPClient.h>
#include <WiFiClient.h>

class BatchUploader {
 public:
  BatchUploader();
  void begin(const char* baseUrl, const char* deviceId, const char* apiKey);
  void addSample(const VoltageSample& sample);
  void addEvent(const VoltageEvent& event);
  void update(bool wifiConnected, uint32_t samplePeriodMs);

 private:
  struct RetryState {
    unsigned long nextAttemptMs = 0;
    size_t backoffIndex = 0;
  };

  bool sendPayload(const String& endpoint, const String& payload, bool wifiConnected, RetryState& retry);
  String buildSamplesPayload(uint32_t samplePeriodMs);
  String buildEventPayload(const VoltageEvent& event);
  void queueSamplesPayload(const String& payload);
  void queueEventPayload(const String& payload);

  const char* baseUrl_ = nullptr;
  const char* deviceId_ = nullptr;
  const char* apiKey_ = nullptr;

  std::vector<VoltageSample> samples_;
  unsigned long lastBatchMs_ = 0;

  StorageQueue samplesQueue_;
  StorageQueue eventsQueue_;
  RetryState samplesRetry_;
  RetryState eventsRetry_;
};
