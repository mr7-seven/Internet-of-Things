#pragma once

#include <Arduino.h>
#include <AsyncMqttClient.h>
#include <atomic>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

class MqttManager {
 public:
  using MessageHandler = void (*)(const char* topic, const char* payload);

  MqttManager(const char* host,
              uint16_t port,
              const char* actuatorTopic,
              uint32_t reconnectIntervalMs,
              uint32_t connectTimeoutMs);

  void begin(MessageHandler messageHandler);
  void update(bool wifiConnected);
  bool isConnected() const;
  uint16_t publish(const char* topic,
                   const char* payload,
                   size_t length,
                   uint8_t qos = 0,
                   bool retain = false);
  const char* clientId() const;

 private:
  struct ReceivedMessage {
    char topic[128];
    char payload[256];
  };

  static MqttManager* instance_;
  static void handleConnect(bool sessionPresent);
  static void handleDisconnect(AsyncMqttClientDisconnectReason reason);
  static void handleMessage(char* topic,
                            char* payload,
                            AsyncMqttClientMessageProperties properties,
                            size_t length,
                            size_t index,
                            size_t total);

  void connect(uint32_t now);
  void processMessageChunk(char* topic,
                           char* payload,
                           size_t length,
                           size_t index,
                           size_t total);

  AsyncMqttClient mqttClient_;
  QueueHandle_t messageQueue_ = nullptr;
  MessageHandler messageHandler_ = nullptr;

  const char* host_;
  uint16_t port_;
  const char* actuatorTopic_;
  uint32_t reconnectIntervalMs_;
  uint32_t connectTimeoutMs_;

  char clientId_[24]{};
  char receiveTopic_[128]{};
  char receivePayload_[256]{};
  bool invalidMessage_ = false;

  std::atomic<uint32_t> connectionStartedAt_{0};
  std::atomic<uint32_t> nextReconnectAt_{0};
  std::atomic<uint8_t> disconnectReason_{0};
  std::atomic<bool> connecting_{false};
  std::atomic<bool> connected_{false};
  std::atomic<bool> reconnectPending_{false};
  std::atomic<bool> connectedEvent_{false};
  std::atomic<bool> disconnectedEvent_{false};
};
