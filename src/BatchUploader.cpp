#include "BatchUploader.h"

namespace {
constexpr unsigned long kBackoffScheduleMs[] = {60000, 120000, 300000, 600000};
}

BatchUploader::BatchUploader()
    : samplesQueue_("/samples_queue.txt"), eventsQueue_("/events_queue.txt") {}

void BatchUploader::begin(const char* baseUrl, const char* deviceId, const char* apiKey) {
  baseUrl_ = baseUrl;
  deviceId_ = deviceId;
  apiKey_ = apiKey;
  samplesQueue_.begin();
  eventsQueue_.begin();
  lastBatchMs_ = millis();
}

void BatchUploader::addSample(const VoltageSample& sample) {
  samples_.push_back(sample);
}

void BatchUploader::addEvent(const VoltageEvent& event) {
  String payload = buildEventPayload(event);
  queueEventPayload(payload);
}

void BatchUploader::update(bool wifiConnected, uint32_t samplePeriodMs) {
  unsigned long now = millis();

  if (!samples_.empty()) {
    bool batchReady = samples_.size() >= Config::kBatchMaxPoints || (now - lastBatchMs_ >= Config::kBatchMaxWaitMs);
    if (batchReady) {
      String payload = buildSamplesPayload(samplePeriodMs);
      queueSamplesPayload(payload);
      samples_.clear();
      lastBatchMs_ = now;
    }
  }

  if (samplesQueue_.hasItems()) {
    String payload = samplesQueue_.front();
    if (sendPayload("/ingest/voltage/samples", payload, wifiConnected, samplesRetry_)) {
      samplesQueue_.pop();
    }
  }

  if (eventsQueue_.hasItems()) {
    String payload = eventsQueue_.front();
    if (sendPayload("/ingest/voltage/events", payload, wifiConnected, eventsRetry_)) {
      eventsQueue_.pop();
    }
  }
}

bool BatchUploader::sendPayload(const String& endpoint, const String& payload, bool wifiConnected, RetryState& retry) {
  unsigned long now = millis();
  if (!wifiConnected) {
    retry.nextAttemptMs = now + kBackoffScheduleMs[0];
    return false;
  }
  if (now < retry.nextAttemptMs) {
    return false;
  }

  WiFiClient client;
  HTTPClient http;
  String url = String(baseUrl_) + endpoint;
  http.begin(client, url);
  http.addHeader("Content-Type", "application/json");
  http.addHeader("X-DEVICE-ID", deviceId_);
  if (apiKey_ != nullptr && strlen(apiKey_) > 0) {
    http.addHeader("X-API-KEY", apiKey_);
  }

  int httpCode = http.POST(payload);
  http.end();

  if (httpCode >= 200 && httpCode < 300) {
    retry.backoffIndex = 0;
    retry.nextAttemptMs = now;
    Serial.printf("[UPLOAD] Success %s (%d)\n", endpoint.c_str(), httpCode);
    return true;
  }

  size_t index = retry.backoffIndex;
  if (index + 1 < (sizeof(kBackoffScheduleMs) / sizeof(kBackoffScheduleMs[0]))) {
    retry.backoffIndex++;
  }
  retry.nextAttemptMs = now + kBackoffScheduleMs[retry.backoffIndex];
  Serial.printf("[UPLOAD] Failed %s (%d). Retry in %lu ms\n", endpoint.c_str(), httpCode, retry.nextAttemptMs - now);
  return false;
}

String BatchUploader::buildSamplesPayload(uint32_t samplePeriodMs) {
  String payload;
  payload.reserve(samples_.size() * 24 + 128);
  payload += "{\"device_id\":\"";
  payload += deviceId_;
  payload += "\",\"fw_version\":\"";
  payload += Config::kFirmwareVersion;
  payload += "\",\"sample_period_ms\":";
  payload += String(samplePeriodMs);
  payload += ",\"samples\":[";

  for (size_t i = 0; i < samples_.size(); ++i) {
    const auto& sample = samples_[i];
    float vrms = (sample.flags & FLAG_NO_SIGNAL) ? 0.0f : sample.vrms;
    payload += "[";
    payload += String(static_cast<uint64_t>(sample.ts_ms));
    payload += ",";
    payload += String(vrms, 3);
    payload += ",";
    payload += String(sample.flags);
    payload += "]";
    if (i + 1 < samples_.size()) {
      payload += ",";
    }
  }
  payload += "]}";
  return payload;
}

String BatchUploader::buildEventPayload(const VoltageEvent& event) {
  String payload;
  payload.reserve(event.samples.size() * 24 + 256);
  payload += "{\"device_id\":\"";
  payload += deviceId_;
  payload += "\",\"fw_version\":\"";
  payload += Config::kFirmwareVersion;
  payload += "\",\"type\":\"";
  payload += EventTypeToString(event.type);
  payload += "\",\"start_ts\":";
  payload += String(static_cast<uint64_t>(event.start_ts));
  payload += ",\"end_ts\":";
  payload += String(static_cast<uint64_t>(event.end_ts));
  payload += ",\"min_vrms\":";
  payload += String(event.min_vrms, 3);
  payload += ",\"max_vrms\":";
  payload += String(event.max_vrms, 3);
  payload += ",\"samples\":[";

  for (size_t i = 0; i < event.samples.size(); ++i) {
    const auto& sample = event.samples[i];
    float vrms = (sample.flags & FLAG_NO_SIGNAL) ? 0.0f : sample.vrms;
    payload += "[";
    payload += String(static_cast<uint64_t>(sample.ts_ms));
    payload += ",";
    payload += String(vrms, 3);
    payload += ",";
    payload += String(sample.flags);
    payload += "]";
    if (i + 1 < event.samples.size()) {
      payload += ",";
    }
  }
  payload += "]}";
  return payload;
}

void BatchUploader::queueSamplesPayload(const String& payload) {
  samplesQueue_.enqueue(payload);
}

void BatchUploader::queueEventPayload(const String& payload) {
  eventsQueue_.enqueue(payload);
}
