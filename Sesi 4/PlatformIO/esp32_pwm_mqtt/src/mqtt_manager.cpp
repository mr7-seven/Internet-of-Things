#include "mqtt_manager.h"

#include <WiFi.h>
#include <esp_system.h>
#include <cstring>

MqttManager* MqttManager::instance_ = nullptr;

MqttManager::MqttManager(const char* host,
                         uint16_t port,
                         const char* actuatorTopic,
                         uint32_t reconnectIntervalMs,
                         uint32_t connectTimeoutMs)
    : host_(host),
      port_(port),
      actuatorTopic_(actuatorTopic),
      reconnectIntervalMs_(reconnectIntervalMs),
      connectTimeoutMs_(connectTimeoutMs) {}

void MqttManager::begin(MessageHandler messageHandler) {
  instance_ = this;
  messageHandler_ = messageHandler;
  messageQueue_ = xQueueCreate(4, sizeof(ReceivedMessage));

  const uint32_t chipId = static_cast<uint32_t>(ESP.getEfuseMac());
  const uint16_t randomId = static_cast<uint16_t>(esp_random());
  snprintf(clientId_, sizeof(clientId_), "esp32-%08lX-%04X",
           static_cast<unsigned long>(chipId), randomId);

  mqttClient_.setServer(host_, port_);
  mqttClient_.setClientId(clientId_);
  mqttClient_.setKeepAlive(30);
  mqttClient_.setCleanSession(true);
  mqttClient_.onConnect(handleConnect);
  mqttClient_.onDisconnect(handleDisconnect);
  mqttClient_.onMessage(handleMessage);
}

void MqttManager::update(bool wifiConnected) {
  const uint32_t now = millis();

  if (messageQueue_ != nullptr && messageHandler_ != nullptr) {
    ReceivedMessage message{};

    while (xQueueReceive(messageQueue_, &message, 0) == pdTRUE) {
      messageHandler_(message.topic, message.payload);
    }
  }

  if (connectedEvent_.exchange(false)) {
    Serial.printf("[MQTT] Connected as %s\n", clientId_);
  }

  if (disconnectedEvent_.exchange(false)) {
    Serial.printf("[MQTT] Disconnected, reason: %u\n",
                  static_cast<unsigned>(disconnectReason_.load()));
  }

  if (!wifiConnected) {
    connected_ = false;
    connecting_ = false;
    reconnectPending_ = false;
    return;
  }

  if (connected_.load() || mqttClient_.connected()) {
    connected_ = true;
    connecting_ = false;
    reconnectPending_ = false;
    return;
  }

  if (!connecting_.load() && !reconnectPending_.load()) {
    reconnectPending_ = true;
    nextReconnectAt_ = now;
  }

  if (connecting_.load() &&
      now - connectionStartedAt_.load() >= connectTimeoutMs_) {
    Serial.println("[MQTT] Connection timeout");
    mqttClient_.disconnect(true);
    connecting_ = false;
    reconnectPending_ = true;
    nextReconnectAt_ = now + reconnectIntervalMs_;
  }

  if (reconnectPending_.load() && !connecting_.load() &&
      static_cast<int32_t>(now - nextReconnectAt_.load()) >= 0) {
    connect(now);
  }
}

bool MqttManager::isConnected() const {
  return connected_.load();
}

uint16_t MqttManager::publish(const char* topic,
                              const char* payload,
                              size_t length,
                              uint8_t qos,
                              bool retain) {
  if (!isConnected() || topic == nullptr || payload == nullptr || length == 0) {
    return 0;
  }

  return mqttClient_.publish(topic, qos, retain, payload, length);
}

const char* MqttManager::clientId() const {
  return clientId_;
}

void MqttManager::connect(uint32_t now) {
  Serial.printf("[MQTT] Connecting to %s:%u\n", host_, port_);
  connectionStartedAt_ = now;
  connecting_ = true;
  reconnectPending_ = false;
  mqttClient_.connect();
}

void MqttManager::handleConnect(bool sessionPresent) {
  (void)sessionPresent;

  if (instance_ == nullptr) {
    return;
  }

  instance_->connected_ = true;
  instance_->connecting_ = false;
  instance_->reconnectPending_ = false;
  instance_->connectedEvent_ = true;
  instance_->mqttClient_.subscribe(instance_->actuatorTopic_, 1);
}

void MqttManager::handleDisconnect(AsyncMqttClientDisconnectReason reason) {
  if (instance_ == nullptr) {
    return;
  }

  instance_->connected_ = false;
  instance_->connecting_ = false;
  instance_->reconnectPending_ = true;
  instance_->nextReconnectAt_ =
      millis() + instance_->reconnectIntervalMs_;
  instance_->disconnectReason_ = static_cast<uint8_t>(reason);
  instance_->disconnectedEvent_ = true;
}

void MqttManager::handleMessage(
    char* topic,
    char* payload,
    AsyncMqttClientMessageProperties properties,
    size_t length,
    size_t index,
    size_t total) {
  (void)properties;

  if (instance_ != nullptr) {
    instance_->processMessageChunk(topic, payload, length, index, total);
  }
}

void MqttManager::processMessageChunk(char* topic,
                                      char* payload,
                                      size_t length,
                                      size_t index,
                                      size_t total) {
  if (index == 0) {
    invalidMessage_ = total >= sizeof(receivePayload_) ||
                      strlen(topic) >= sizeof(receiveTopic_);

    if (!invalidMessage_) {
      strncpy(receiveTopic_, topic, sizeof(receiveTopic_) - 1);
      receiveTopic_[sizeof(receiveTopic_) - 1] = '\0';
    }
  }

  if (invalidMessage_ || index + length > sizeof(receivePayload_) - 1) {
    return;
  }

  memcpy(receivePayload_ + index, payload, length);

  if (index + length == total) {
    receivePayload_[total] = '\0';

    if (messageQueue_ != nullptr) {
      ReceivedMessage message{};
      strncpy(message.topic, receiveTopic_, sizeof(message.topic) - 1);
      strncpy(message.payload, receivePayload_, sizeof(message.payload) - 1);
      xQueueSend(messageQueue_, &message, 0);
    }
  }
}
