#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <atomic>

class WifiManager {
 public:
  WifiManager(const char* ssid,
              const char* password,
              uint32_t reconnectIntervalMs = 5000,
              uint32_t connectTimeoutMs = 15000);

  void begin();
  void update();
  bool isConnected() const;
  IPAddress localIP() const;

 private:
  static WifiManager* instance_;
  static void handleEvent(WiFiEvent_t event, WiFiEventInfo_t info);

  void startConnection(uint32_t now);

  const char* ssid_;
  const char* password_;
  uint32_t reconnectIntervalMs_;
  uint32_t connectTimeoutMs_;
  std::atomic<uint32_t> connectionStartedAt_{0};
  std::atomic<uint32_t> nextReconnectAt_{0};
  std::atomic<bool> connecting_{false};
  std::atomic<bool> reconnectPending_{false};
};
