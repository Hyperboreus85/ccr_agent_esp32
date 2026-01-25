#include <Arduino.h>
#include <WiFi.h>

namespace {
constexpr uint8_t kAdcPin = 32; // GPIO32 (ADC1)
constexpr uint32_t kSampleWindowMs = 1000;
constexpr uint32_t kPrintIntervalMs = 1000;
constexpr uint32_t kSampleIntervalUs = 200; // ~5000 samples/sec
constexpr float kAdcRefVoltage = 3.3f;
constexpr float kAdcMax = 4095.0f;
constexpr float kCalibration = 1.0f; // adjust after calibration

constexpr const char* kWifiSsid = "WIFI";
constexpr const char* kWifiPassword = "wifi1234";
}

unsigned long lastPrintMs = 0;

void setup() {
  Serial.begin(115200);
  delay(500);

  analogReadResolution(12);
  analogSetPinAttenuation(kAdcPin, ADC_11db);

  WiFi.mode(WIFI_STA);
  WiFi.begin(kWifiSsid, kWifiPassword);

  Serial.println("[BOOT] ESP32 ZMPT101B debug sketch");
  Serial.printf("[WIFI] Connecting to %s...\n", kWifiSsid);
}

void loop() {
  unsigned long windowStart = millis();
  unsigned long lastSampleUs = micros();

  uint16_t rawMin = 4095;
  uint16_t rawMax = 0;
  uint64_t sum = 0;
  uint64_t sumSq = 0;
  uint32_t count = 0;

  while (millis() - windowStart < kSampleWindowMs) {
    unsigned long nowUs = micros();
    if (nowUs - lastSampleUs >= kSampleIntervalUs) {
      lastSampleUs = nowUs;
      uint16_t raw = analogRead(kAdcPin);
      rawMin = min(rawMin, raw);
      rawMax = max(rawMax, raw);
      sum += raw;
      sumSq += static_cast<uint64_t>(raw) * raw;
      count++;
    }
  }

  if (count == 0) {
    return;
  }

  float rawAvg = static_cast<float>(sum) / static_cast<float>(count);
  float meanSq = static_cast<float>(sumSq) / static_cast<float>(count);
  float variance = meanSq - rawAvg * rawAvg;
  if (variance < 0.0f) {
    variance = 0.0f;
  }
  float rmsRaw = sqrtf(variance);
  float vAdc = rmsRaw * (kAdcRefVoltage / kAdcMax);
  float vMains = vAdc * kCalibration;
  float amp = (static_cast<float>(rawMax) - static_cast<float>(rawMin)) / 2.0f;

  unsigned long nowMs = millis();
  if (nowMs - lastPrintMs >= kPrintIntervalMs) {
    Serial.printf(
        "rawMin=%u rawMax=%u offset=%.2f amp=%.2f vrms_adc=%.4f vrms_mains=%.2f wifi=%s ip=%s\n",
        rawMin,
        rawMax,
        rawAvg,
        amp,
        vAdc,
        vMains,
        WiFi.status() == WL_CONNECTED ? "connected" : "disconnected",
        WiFi.status() == WL_CONNECTED ? WiFi.localIP().toString().c_str() : "-");
    lastPrintMs = nowMs;
  }

  if (WiFi.status() == WL_CONNECTED) {
    static bool loggedIp = false;
    if (!loggedIp) {
      Serial.printf("[WIFI] Connected. IP: %s\n", WiFi.localIP().toString().c_str());
      loggedIp = true;
    }
  }
}
