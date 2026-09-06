#include <Arduino.h>
#include <ArduinoJson.h>
#include <esp_system.h>

#include "app_config.h"
#include "http_manager.h"
#include "wifi_manager.h"

WifiManager wifiManager(AppConfig::WIFI_SSID,
                        AppConfig::WIFI_PASSWORD,
                        AppConfig::WIFI_RECONNECT_INTERVAL_MS,
                        AppConfig::WIFI_CONNECT_TIMEOUT_MS);

HttpManager httpManager(AppConfig::HTTP_URL, AppConfig::HTTP_TIMEOUT_MS);

uint32_t lastSendAt = 0;

String createSensorPayload() {
  const float temperature = random(2000, 3501) / 100.0F;
  const float humidity = random(4000, 8001) / 100.0F;

  JsonDocument document;
  document["temperature"] = temperature;
  document["humidity"] = humidity;

  String payload;
  serializeJson(document, payload);
  return payload;
}

void setup() {
  Serial.begin(115200);
  randomSeed(esp_random());
  wifiManager.begin();
}

void loop() {
  wifiManager.update();

  const uint32_t now = millis();
  if (now - lastSendAt < AppConfig::SEND_INTERVAL_MS) {
    return;
  }

  lastSendAt = now;
  if (!wifiManager.isConnected()) {
    Serial.println("[APP] Send skipped: WiFi not connected");
    return;
  }

  const String payload = createSensorPayload();
  Serial.printf("[APP] Payload: %s\n", payload.c_str());
  httpManager.sendPayload(payload);
}

