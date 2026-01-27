#pragma once

#include "Config.h"

#include <deque>

class EventDetector {
 public:
  EventDetector();
  void addSample(const VoltageSample& sample);
  bool pollCompletedEvent(VoltageEvent& eventOut);

 private:
  float detectionValue() const;
  void startEvent(EventType type, const VoltageSample& sample);
  void appendSampleToEvent(const VoltageSample& sample);
  void finalizeEvent();

  std::vector<VoltageSample> ringBuffer_;
  size_t ringIndex_ = 0;
  bool ringFull_ = false;

  uint16_t sagCounter_ = 0;
  uint16_t swellCounter_ = 0;

  bool eventActive_ = false;
  bool postRecording_ = false;
  EventType activeType_ = EventType::Sag;
  VoltageEvent activeEvent_;
  uint16_t endCounter_ = 0;
  uint16_t postCounter_ = 0;

  bool completedReady_ = false;
  VoltageEvent completedEvent_;
};
