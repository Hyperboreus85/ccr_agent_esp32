#pragma once

#include <Arduino.h>
#include <LittleFS.h>
#include <vector>

class StorageQueue {
 public:
  explicit StorageQueue(const char* path);
  bool begin();
  void enqueue(const String& payload);
  bool hasItems() const;
  String front() const;
  void pop();

 private:
  void persist();

  const char* path_;
  std::vector<String> items_;
};
