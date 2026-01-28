#include "EventDetector.h"

#include <algorithm>

EventDetector::EventDetector() {
  ringBuffer_.resize(Config::kRingBufferPoints);
}

void EventDetector::addSample(const VoltageSample& sample) {
  ringBuffer_[ringIndex_] = sample;
  ringIndex_ = (ringIndex_ + 1) % ringBuffer_.size();
  if (ringIndex_ == 0) {
    ringFull_ = true;
  }

  if (sample.flags & FLAG_NO_SIGNAL) {
    eventActive_ = false;
    postRecording_ = false;
    sagCounter_ = 0;
    swellCounter_ = 0;
    endCounter_ = 0;
    postCounter_ = 0;
    return;
  }

  float detectVrms = detectionValue();

  if (eventActive_) {
    appendSampleToEvent(sample);

    bool endCondition = false;
    if (activeType_ == EventType::Sag) {
      if (detectVrms > Config::kSagEnd) {
        endCounter_++;
      } else {
        endCounter_ = 0;
      }
      endCondition = endCounter_ >= Config::kSagEndWindows;
    } else if (activeType_ == EventType::Swell) {
      if (detectVrms < Config::kSwellEnd) {
        endCounter_++;
      } else {
        endCounter_ = 0;
      }
      endCondition = endCounter_ >= Config::kSwellEndWindows;
    } else {
      if (detectVrms > Config::kCriticalLow && detectVrms < Config::kCriticalHigh) {
        endCounter_++;
      } else {
        endCounter_ = 0;
      }
      endCondition = endCounter_ >= Config::kCriticalEndWindows;
    }

    if (endCondition && !postRecording_) {
      activeEvent_.end_ts = sample.ts_ms;
      postRecording_ = true;
      postCounter_ = 0;
    }

    if (postRecording_) {
      postCounter_++;
      if (postCounter_ >= Config::kEventPostPoints) {
        finalizeEvent();
      }
    }
    return;
  }

  if (detectVrms < Config::kCriticalLow || detectVrms > Config::kCriticalHigh) {
    startEvent(EventType::Critical, sample);
    return;
  }

  if (detectVrms < Config::kSagStart) {
    sagCounter_++;
  } else {
    sagCounter_ = 0;
  }

  if (detectVrms > Config::kSwellStart) {
    swellCounter_++;
  } else {
    swellCounter_ = 0;
  }

  if (sagCounter_ >= Config::kSagStartWindows) {
    startEvent(EventType::Sag, sample);
    sagCounter_ = 0;
  } else if (swellCounter_ >= Config::kSwellStartWindows) {
    startEvent(EventType::Swell, sample);
    swellCounter_ = 0;
  }
}

bool EventDetector::pollCompletedEvent(VoltageEvent& eventOut) {
  if (!completedReady_) {
    return false;
  }
  eventOut = completedEvent_;
  completedReady_ = false;
  return true;
}

float EventDetector::detectionValue() const {
  float sum = 0.0f;
  int count = 0;
  size_t available = ringFull_ ? ringBuffer_.size() : ringIndex_;
  size_t limit = std::min<size_t>(3, available);
  for (size_t i = 0; i < limit; ++i) {
    size_t index = (ringIndex_ + ringBuffer_.size() - 1 - i) % ringBuffer_.size();
    if (ringBuffer_[index].flags & FLAG_NO_SIGNAL) {
      continue;
    }
    sum += ringBuffer_[index].vrms;
    count++;
  }
  if (count == 0) {
    return 0.0f;
  }
  return sum / static_cast<float>(count);
}

void EventDetector::startEvent(EventType type, const VoltageSample& sample) {
  eventActive_ = true;
  postRecording_ = false;
  activeType_ = type;
  endCounter_ = 0;
  postCounter_ = 0;

  activeEvent_ = {};
  activeEvent_.type = type;
  activeEvent_.start_ts = sample.ts_ms;
  activeEvent_.min_vrms = sample.vrms;
  activeEvent_.max_vrms = sample.vrms;
  activeEvent_.samples.clear();

  size_t count = ringFull_ ? ringBuffer_.size() : ringIndex_;
  size_t available = std::min(count, static_cast<size_t>(Config::kEventPrePoints));
  activeEvent_.samples.reserve(available + Config::kEventPostPoints + 10);

  size_t startIndex = (ringIndex_ + ringBuffer_.size() - available) % ringBuffer_.size();
  for (size_t i = 0; i < available; ++i) {
    size_t idx = (startIndex + i) % ringBuffer_.size();
    activeEvent_.samples.push_back(ringBuffer_[idx]);
  }
  appendSampleToEvent(sample);
}

void EventDetector::appendSampleToEvent(const VoltageSample& sample) {
  activeEvent_.samples.push_back(sample);
  activeEvent_.min_vrms = std::min(activeEvent_.min_vrms, sample.vrms);
  activeEvent_.max_vrms = std::max(activeEvent_.max_vrms, sample.vrms);
}

void EventDetector::finalizeEvent() {
  completedEvent_ = activeEvent_;
  completedReady_ = true;
  eventActive_ = false;
  postRecording_ = false;
}
