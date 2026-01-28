#include <Arduino.h>
#include <WiFi.h>

namespace {
constexpr uint8_t kAdcPin = 34; // GPIO34 (ADC1)
constexpr uint32_t kSampleWindowMs = 200;
constexpr uint32_t kPrintIntervalMs = 1000;
constexpr uint32_t kSampleIntervalUs = 200; // ~5000 samples/sec
constexpr float kAdcRefVoltage = 3.3f;
constexpr float kAdcMax = 4095.0f;
constexpr float kCalibration = 1.0f; // adjust after calibration
constexpr uint16_t kNoSignalThreshold = 8; // ADC counts

constexpr const char* kWifiSsid = "WIFI";
constexpr const char* kWifiPassword = "wifi1234";

constexpr size_t kMaxSamples = 2000;
}

enum SampleFlags : uint32_t {
  FLAG_NONE = 0,
  FLAG_NO_SIGNAL = 1 << 0,
  FLAG_ADC_SATURATED = 1 << 1,
};

unsigned long lastPrintMs = 0;
float calibGain = kCalibration;
float calibOffset = 0.0f;
bool assistMode = false;
String inputLine;

void handleCommand(const String& line) {
  String cmd = line;
  cmd.trim();
  if (cmd.length() == 0) {
    return;
  }

  if (cmd.equalsIgnoreCase("calib show")) {
    Serial.printf("[CALIB] gain=%.4f offset=%.4f\n", calibGain, calibOffset);
    return;
  }

  if (cmd.startsWith("calib gain")) {
    float value = cmd.substring(String("calib gain").length()).toFloat();
    calibGain = value;
    Serial.printf("[CALIB] gain set to %.4f\n", calibGain);
    return;
  }

  if (cmd.startsWith("calib offset")) {
    float value = cmd.substring(String("calib offset").length()).toFloat();
    calibOffset = value;
    Serial.printf("[CALIB] offset set to %.4f\n", calibOffset);
    return;
  }

  if (cmd.equalsIgnoreCase("calib assist on")) {
    assistMode = true;
    Serial.println("[CALIB] assist ON");
    return;
  }

  if (cmd.equalsIgnoreCase("calib assist off")) {
    assistMode = false;
    Serial.println("[CALIB] assist OFF");
    return;
  }

  if (cmd.equalsIgnoreCase("help")) {
    Serial.println("Commands: calib show | calib gain <v> | calib offset <v> | calib assist on/off");
    return;
  }

  Serial.println("Unknown command. Type 'help'.");
}

void setup() {
  Serial.begin(115200);
  delay(500);

  analogReadResolution(12);
  analogSetPinAttenuation(kAdcPin, ADC_11db);

  WiFi.mode(WIFI_STA);
  WiFi.begin(kWifiSsid, kWifiPassword);

  Serial.println("[BOOT] ESP32 ZMPT101B debug sketch");
  Serial.printf("[WIFI] Connecting to %s...\n", kWifiSsid);
  Serial.println("[SYSTEM] Type 'help' for commands.");
}

void loop() {
  while (Serial.available()) {
    char c = static_cast<char>(Serial.read());
    if (c == '\n') {
      handleCommand(inputLine);
      inputLine = "";
    } else if (c != '\r') {
      inputLine += c;
    }
  }

  static uint16_t samples[kMaxSamples];
  unsigned long windowStart = millis();
  unsigned long lastSampleUs = micros();

  uint16_t rawMin = 4095;
  uint16_t rawMax = 0;
  uint64_t sum = 0;
  uint32_t count = 0;
  uint16_t saturatedCount = 0;

  while (millis() - windowStart < kSampleWindowMs) {
    unsigned long nowUs = micros();
    if (nowUs - lastSampleUs >= kSampleIntervalUs) {
      lastSampleUs = nowUs;
      uint16_t raw = analogRead(kAdcPin);
      if (count < kMaxSamples) {
        samples[count] = raw;
      }
      rawMin = min(rawMin, raw);
      rawMax = max(rawMax, raw);
      sum += raw;
      if (raw == 0 || raw >= 4095) {
        saturatedCount++;
      }
      count++;
    }
  }

  if (count == 0) {
    return;
  }

  float mean = static_cast<float>(sum) / static_cast<float>(count);
  float sumSq = 0.0f;
  // RMS AC: remove DC bias by subtracting the window mean.
  for (uint32_t i = 0; i < count && i < kMaxSamples; ++i) {
    float centered = static_cast<float>(samples[i]) - mean;
    sumSq += centered * centered;
  }

  float rawRms = sqrtf(sumSq / static_cast<float>(count));
  uint16_t pkpk = rawMax - rawMin;
  uint32_t flags = FLAG_NONE;
  if (pkpk < kNoSignalThreshold) {
    rawRms = 0.0f;
    flags |= FLAG_NO_SIGNAL;
  }
  if (saturatedCount > 0) {
    flags |= FLAG_ADC_SATURATED;
  }

  float vAdc = rawRms * (kAdcRefVoltage / kAdcMax);
  float vMains = vAdc * calibGain + calibOffset;
  if (flags != FLAG_NONE) {
    vMains = 0.0f;
  }
  float amp = (static_cast<float>(rawMax) - static_cast<float>(rawMin)) / 2.0f;

  unsigned long nowMs = millis();
  if (nowMs - lastPrintMs >= kPrintIntervalMs) {
    Serial.printf(
        "rawMin=%u rawMax=%u offset=%.2f amp=%.2f vrms_adc=%.4f vrms_mains=%.2f flags=0x%02lx wifi=%s ip=%s\n",
        rawMin,
        rawMax,
        mean,
        amp,
        vAdc,
        vMains,
        flags,
        WiFi.status() == WL_CONNECTED ? "connected" : "disconnected",
        WiFi.status() == WL_CONNECTED ? WiFi.localIP().toString().c_str() : "-");
    lastPrintMs = nowMs;
  }

  if (assistMode) {
    Serial.printf("[ASSIST] raw_rms=%.4f pkpk=%u mean=%.2f\n", rawRms, pkpk, mean);
  }

  if (WiFi.status() == WL_CONNECTED) {
    static bool loggedIp = false;
    if (!loggedIp) {
      Serial.printf("[WIFI] Connected. IP: %s\n", WiFi.localIP().toString().c_str());
      loggedIp = true;
    }
  }
}
