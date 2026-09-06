#pragma once

#include <Arduino.h>

class HttpManager {
 public:
  static constexpr int NOT_CONNECTED = -1000;
  static constexpr int EMPTY_PAYLOAD = -1001;

  HttpManager(const char* url, uint32_t timeoutMs = 5000);

  int sendPayload(const String& payload) const;

 private:
  void printResult(int statusCode) const;

  const char* url_;
  uint32_t timeoutMs_;
};

