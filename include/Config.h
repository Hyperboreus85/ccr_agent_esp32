#pragma once

#include <Arduino.h>
#include <vector>

#if defined(__has_include)
#if __has_include("BuildInfo.h")
#include "BuildInfo.h"
#endif
#endif

#ifndef CCR_FW_VERSION_STR
#define CCR_FW_VERSION_STR "0.1.0.0000000"
#endif

namespace Config {
constexpr const char* kFirmwareVersion = CCR_FW_VERSION_STR;

constexpr uint32_t kSampleRateHz = 2500;
constexpr uint32_t kWindowMs = 200;
constexpr uint32_t kRingBufferSeconds = 60;
constexpr uint32_t kRingBufferPoints = (kRingBufferSeconds * 1000) / kWindowMs;
constexpr uint32_t kEventPreSeconds = 60;
constexpr uint32_t kEventPostSeconds = 60;
constexpr uint32_t kEventPrePoints = (kEventPreSeconds * 1000) / kWindowMs;
constexpr uint32_t kEventPostPoints = (kEventPostSeconds * 1000) / kWindowMs;

constexpr uint32_t kBatchMaxPoints = (30 * 60 * 1000) / kWindowMs; // 30 minutes
constexpr uint32_t kBatchMaxWaitMs = 30 * 60 * 1000;

constexpr uint32_t kNtpResyncMs = 6UL * 60UL * 60UL * 1000UL;

constexpr gpio_num_t kDefaultAdcPin = GPIO_NUM_34;

constexpr float kSagStart = 207.0f;
constexpr float kSagEnd = 210.0f;
constexpr float kSwellStart = 253.0f;
constexpr float kSwellEnd = 250.0f;
constexpr float kCriticalLow = 190.0f;
constexpr float kCriticalHigh = 265.0f;

constexpr uint16_t kSagStartWindows = 5;   // 1s / 200ms
constexpr uint16_t kSagEndWindows = 50;    // 10s / 200ms
constexpr uint16_t kSwellStartWindows = 5;
constexpr uint16_t kSwellEndWindows = 50;
constexpr uint16_t kCriticalEndWindows = 50;

constexpr float kAdcSaturationThreshold = 0.05f; // 5% samples saturated

constexpr float kNoSignalVrms = 10.0f;
constexpr uint16_t kNoSignalRawPkPk = 8; // Optional noise guard (ADC counts).
} // namespace Config

enum SampleFlags : uint16_t {
  FLAG_NONE = 0,
  FLAG_ADC_SATURATED = 1u << 0,
  FLAG_NTP_NOT_SYNC = 1u << 1,
  FLAG_CALIB_MISSING = 1u << 2,
  FLAG_OUT_OF_RANGE_SENSOR = 1u << 3,
  FLAG_WIFI_DOWN = 1u << 4,
  FLAG_NO_SIGNAL = 1u << 5,
};

struct VoltageSample {
  uint64_t ts_ms = 0;
  float vrms = 0.0f;
  float vmin = 0.0f;
  float vmax = 0.0f;
  uint16_t sample_count = 0;
  uint16_t flags = FLAG_NONE;
  float raw_rms = 0.0f;
};

enum class EventType {
  Sag,
  Swell,
  Critical,
};

struct VoltageEvent {
  EventType type = EventType::Sag;
  uint64_t start_ts = 0;
  uint64_t end_ts = 0;
  float min_vrms = 0.0f;
  float max_vrms = 0.0f;
  std::vector<VoltageSample> samples;
};

inline const char* EventTypeToString(EventType type) {
  switch (type) {
    case EventType::Sag:
      return "SAG";
    case EventType::Swell:
      return "SWELL";
    case EventType::Critical:
      return "CRITICAL";
    default:
      return "UNKNOWN";
  }
}
