#include "StorageQueue.h"

StorageQueue::StorageQueue(const char* path) : path_(path) {}

bool StorageQueue::begin() {
  if (!LittleFS.exists(path_)) {
    return true;
  }
  File file = LittleFS.open(path_, "r");
  if (!file) {
    return false;
  }
  while (file.available()) {
    String line = file.readStringUntil('\n');
    line.trim();
    if (line.length() > 0) {
      items_.push_back(line);
    }
  }
  file.close();
  return true;
}

void StorageQueue::enqueue(const String& payload) {
  items_.push_back(payload);
  persist();
}

bool StorageQueue::hasItems() const {
  return !items_.empty();
}

String StorageQueue::front() const {
  if (items_.empty()) {
    return String();
  }
  return items_.front();
}

void StorageQueue::pop() {
  if (items_.empty()) {
    return;
  }
  items_.erase(items_.begin());
  persist();
}

void StorageQueue::persist() {
  File file = LittleFS.open(path_, "w");
  if (!file) {
    return;
  }
  for (const auto& item : items_) {
    file.println(item);
  }
  file.close();
}
