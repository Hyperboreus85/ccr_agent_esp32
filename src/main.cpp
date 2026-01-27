#include <Arduino.h>
#include <Preferences.h>
#include <LittleFS.h>

#include "BatchUploader.h"
#include "Config.h"
#include "EventDetector.h"
#include "TimeSync.h"
#include "VoltageSampler.h"
#include "WifiManager.h"

#include "secrets.h"

namespace {
constexpr uint32_t kSampleLogIntervalMs = 5000;
}

Preferences prefs;
WifiManager wifiManager;
TimeSync timeSync;
VoltageSampler sampler(Config::kDefaultAdcPin);
EventDetector eventDetector;
BatchUploader uploader;

float calibGain = 1.0f;
float calibOffset = 0.0f;
bool calibPresent = false;
bool assistedMode = false;
String inputLine;
unsigned long lastLogMs = 0;
bool lastWifiConnected = false;
bool lastNtpSynced = false;

void applyCalibration() {
  sampler.setCalibration(calibGain, calibOffset, calibPresent);
}

void saveCalibration() {
  prefs.putFloat("gain", calibGain);
  prefs.putFloat("offset", calibOffset);
  prefs.putBool("has", calibPresent);
  applyCalibration();
}

void handleCommand(const String& line) {
  String cmd = line;
  cmd.trim();
  if (cmd.length() == 0) {
    return;
  }

  if (cmd.equalsIgnoreCase("calib show")) {
    Serial.printf("[CALIB] gain=%.4f offset=%.4f present=%s\n", calibGain, calibOffset, calibPresent ? "yes" : "no");
    return;
  }

  if (cmd.startsWith("calib gain")) {
    float value = cmd.substring(String("calib gain").length()).toFloat();
    calibGain = value;
    calibPresent = true;
    saveCalibration();
    Serial.printf("[CALIB] gain set to %.4f\n", calibGain);
    return;
  }

  if (cmd.startsWith("calib offset")) {
    float value = cmd.substring(String("calib offset").length()).toFloat();
    calibOffset = value;
    calibPresent = true;
    saveCalibration();
    Serial.printf("[CALIB] offset set to %.4f\n", calibOffset);
    return;
  }

  if (cmd.equalsIgnoreCase("calib assist on")) {
    assistedMode = true;
    Serial.println("[CALIB] assisted mode ON");
    return;
  }

  if (cmd.equalsIgnoreCase("calib assist off")) {
    assistedMode = false;
    Serial.println("[CALIB] assisted mode OFF");
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
  Serial.printf("CCR ESP32 firmware %s\n", Config::kFirmwareVersion);

  prefs.begin("calib", false);
  calibGain = prefs.getFloat("gain", 1.0f);
  calibOffset = prefs.getFloat("offset", 0.0f);
  calibPresent = prefs.getBool("has", false);
  if (!calibPresent) {
    Serial.println("[CALIB] No calibration found. Using defaults.");
  }
  applyCalibration();

  wifiManager.begin(WIFI_SSID, WIFI_PASSWORD);
  timeSync.begin();

  sampler.begin();
  if (!LittleFS.begin(true)) {
    Serial.println("[FS] LittleFS mount failed. Persistence disabled.");
  }
  uploader.begin(CCR_BASE_URL, DEVICE_ID, CCR_API_KEY);

  Serial.println("[SYSTEM] Setup complete. Type 'help' for commands.");
}

void loop() {
  wifiManager.update();
  timeSync.update();

  bool wifiConnected = wifiManager.isConnected();
  if (wifiConnected != lastWifiConnected) {
    Serial.printf("[WIFI] %s\n", wifiConnected ? "connected" : "disconnected");
    lastWifiConnected = wifiConnected;
  }

  bool ntpSynced = timeSync.isSynced();
  if (ntpSynced != lastNtpSynced) {
    Serial.printf("[NTP] %s\n", ntpSynced ? "synced" : "not synced");
    lastNtpSynced = ntpSynced;
  }

  while (Serial.available()) {
    char c = static_cast<char>(Serial.read());
    if (c == '\n') {
      handleCommand(inputLine);
      inputLine = "";
    } else if (c != '\r') {
      inputLine += c;
    }
  }

  VoltageSample sample;
  if (sampler.update(sample)) {
    sample.ts_ms = timeSync.nowMs();
    if (!timeSync.isSynced()) {
      sample.flags |= FLAG_NTP_NOT_SYNC;
    }
    if (!wifiConnected) {
      sample.flags |= FLAG_WIFI_DOWN;
    }

    bool saturated = (sample.flags & FLAG_ADC_SATURATED) != 0;
    if (!saturated) {
      eventDetector.addSample(sample);
      uploader.addSample(sample);
    }

    if (assistedMode) {
      Serial.printf("[ASSIST] raw_rms=%.3f vrms=%.3f\n", sample.raw_rms, sample.vrms);
    }

    if (millis() - lastLogMs > kSampleLogIntervalMs) {
      Serial.printf("[SAMPLE] vrms=%.2f flags=0x%08lx\n", sample.vrms, sample.flags);
      lastLogMs = millis();
    }
  }

  VoltageEvent event;
  if (eventDetector.pollCompletedEvent(event)) {
    Serial.printf("[EVENT] %s start=%llu end=%llu min=%.2f max=%.2f samples=%u\n",
                  EventTypeToString(event.type),
                  static_cast<unsigned long long>(event.start_ts),
                  static_cast<unsigned long long>(event.end_ts),
                  event.min_vrms,
                  event.max_vrms,
                  static_cast<unsigned int>(event.samples.size()));
    uploader.addEvent(event);
  }

  uploader.update(wifiConnected, Config::kWindowMs);
}
