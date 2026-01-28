#pragma once

#include "Config.h"

class VoltageSampler {
 public:
  explicit VoltageSampler(gpio_num_t pin);

  void begin();
  void setCalibration(float gain, float offset, bool hasCalibration);
  bool update(VoltageSample& outSample);
  float lastRawRms() const;
  float lastVrms() const;

 private:
  gpio_num_t adcPin_;
  float gain_ = 1.0f;
  float offset_ = 0.0f;
  bool hasCalibration_ = false;

  unsigned long windowStartMs_ = 0;
  unsigned long lastSampleUs_ = 0;
  uint32_t intervalUs_ = 0;

  uint32_t count_ = 0;
  uint64_t sum_ = 0;
  uint64_t sumSq_ = 0;
  uint16_t saturatedCount_ = 0;
  uint16_t minRaw_ = 4095;
  uint16_t maxRaw_ = 0;

  float lastRawRms_ = 0.0f;
  float lastVrms_ = 0.0f;
  bool lastNoSignal_ = false;
};
