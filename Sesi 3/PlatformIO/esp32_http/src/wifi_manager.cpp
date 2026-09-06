#include "wifi_manager.h"

WifiManager* WifiManager::instance_ = nullptr;

WifiManager::WifiManager(const char* ssid,
                         const char* password,
                         uint32_t reconnectIntervalMs,
                         uint32_t connectTimeoutMs)
    : ssid_(ssid),
      password_(password),
      reconnectIntervalMs_(reconnectIntervalMs),
      connectTimeoutMs_(connectTimeoutMs) {}

void WifiManager::begin() {
  instance_ = this;
  WiFi.onEvent(handleEvent);
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(false);
  WiFi.persistent(false);

  reconnectPending_ = true;
  nextReconnectAt_ = millis();
}

void WifiManager::update() {
  const uint32_t now = millis();

  if (isConnected()) {
    connecting_ = false;
    reconnectPending_ = false;
    return;
  }

  if (connecting_.load() &&
      now - connectionStartedAt_.load() >= connectTimeoutMs_) {
    Serial.println("[WiFi] Connection timeout");
    WiFi.disconnect();
    connecting_ = false;
    reconnectPending_ = true;
    nextReconnectAt_ = now + reconnectIntervalMs_;
  }

  if (reconnectPending_.load() && !connecting_.load() &&
      static_cast<int32_t>(now - nextReconnectAt_.load()) >= 0) {
    startConnection(now);
  }
}

bool WifiManager::isConnected() const {
  return WiFi.status() == WL_CONNECTED;
}

IPAddress WifiManager::localIP() const {
  return WiFi.localIP();
}

void WifiManager::startConnection(uint32_t now) {
  Serial.printf("[WiFi] Connecting to %s\n", ssid_);
  WiFi.begin(ssid_, password_);
  connectionStartedAt_ = now;
  connecting_ = true;
  reconnectPending_ = false;
}

void WifiManager::handleEvent(WiFiEvent_t event, WiFiEventInfo_t info) {
  if (instance_ == nullptr) {
    return;
  }

  switch (event) {
    case ARDUINO_EVENT_WIFI_STA_GOT_IP:
      instance_->connecting_ = false;
      instance_->reconnectPending_ = false;
      Serial.printf("[WiFi] Connected, IP: %s\n",
                    WiFi.localIP().toString().c_str());
      break;

    case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
      instance_->connecting_ = false;
      instance_->reconnectPending_ = true;
      instance_->nextReconnectAt_ = millis() + instance_->reconnectIntervalMs_;
      Serial.printf("[WiFi] Disconnected, reason: %u\n",
                    static_cast<unsigned>(info.wifi_sta_disconnected.reason));
      break;

    default:
      break;
  }
}
