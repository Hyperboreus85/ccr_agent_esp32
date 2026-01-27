#include "VoltageSampler.h"

#include <math.h>

VoltageSampler::VoltageSampler(gpio_num_t pin) : adcPin_(pin) {}

void VoltageSampler::begin() {
  analogReadResolution(12);
  analogSetPinAttenuation(adcPin_, ADC_11db);
  intervalUs_ = 1000000UL / Config::kSampleRateHz;
  windowStartMs_ = millis();
  lastSampleUs_ = micros();
}

void VoltageSampler::setCalibration(float gain, float offset, bool hasCalibration) {
  gain_ = gain;
  offset_ = offset;
  hasCalibration_ = hasCalibration;
}

bool VoltageSampler::update(VoltageSample& outSample) {
  unsigned long nowUs = micros();
  if (nowUs - lastSampleUs_ >= intervalUs_) {
    lastSampleUs_ = nowUs;
    uint16_t raw = analogRead(adcPin_);
    count_++;
    sum_ += raw;
    sumSq_ += static_cast<uint64_t>(raw) * raw;
    if (raw == 0 || raw >= 4095) {
      saturatedCount_++;
    }
    if (raw < minRaw_) {
      minRaw_ = raw;
    }
    if (raw > maxRaw_) {
      maxRaw_ = raw;
    }
  }

  unsigned long nowMs = millis();
  if (nowMs - windowStartMs_ < Config::kWindowMs) {
    return false;
  }

  if (count_ == 0) {
    windowStartMs_ = nowMs;
    return false;
  }

  float mean = static_cast<float>(sum_) / static_cast<float>(count_);
  float meanSq = static_cast<float>(sumSq_) / static_cast<float>(count_);
  float variance = meanSq - mean * mean;
  if (variance < 0.0f) {
    variance = 0.0f;
  }
  lastRawRms_ = sqrtf(variance);
  lastVrms_ = (lastRawRms_ * gain_) + offset_;

  outSample.raw_rms = lastRawRms_;
  outSample.vrms = lastVrms_;
  outSample.vmin = (static_cast<float>(minRaw_) * gain_) + offset_;
  outSample.vmax = (static_cast<float>(maxRaw_) * gain_) + offset_;
  outSample.sample_count = static_cast<uint16_t>(count_);
  outSample.flags = FLAG_NONE;
  if (!hasCalibration_) {
    outSample.flags |= FLAG_CALIB_MISSING;
  }
  if (static_cast<float>(saturatedCount_) / static_cast<float>(count_) > Config::kAdcSaturationThreshold) {
    outSample.flags |= FLAG_ADC_SATURATED;
  }

  count_ = 0;
  sum_ = 0;
  sumSq_ = 0;
  saturatedCount_ = 0;
  minRaw_ = 4095;
  maxRaw_ = 0;
  windowStartMs_ = nowMs;
  return true;
}

float VoltageSampler::lastRawRms() const {
  return lastRawRms_;
}

float VoltageSampler::lastVrms() const {
  return lastVrms_;
}
