#include <Arduino.h>
#include <ArduinoJson.h>
#include <esp_arduino_version.h>
#include <esp_system.h>
#include <cstring>
#include <strings.h>

#include "app_config.h"
#include "mqtt_manager.h"
#include "wifi_manager.h"

namespace {
enum class AppMode : uint8_t {
  ACTUATOR_ONLY,
  SENSOR_AND_ACTUATOR
};

constexpr AppMode APP_MODE = AppMode::ACTUATOR_ONLY;
constexpr bool LOG_REJECTED_COMMANDS = false;
constexpr uint8_t RED_PWM_CHANNEL = 0;
constexpr uint8_t YELLOW_PWM_CHANNEL = 1;
constexpr uint8_t GREEN_PWM_CHANNEL = 2;
}

WifiManager wifiManager(AppConfig::WIFI_SSID,
                        AppConfig::WIFI_PASSWORD,
                        AppConfig::WIFI_RECONNECT_INTERVAL_MS,
                        AppConfig::WIFI_CONNECT_TIMEOUT_MS);

MqttManager mqttManager(AppConfig::MQTT_HOST,
                        AppConfig::MQTT_PORT,
                        AppConfig::MQTT_ACTUATOR_TOPIC,
                        AppConfig::MQTT_RECONNECT_INTERVAL_MS,
                        AppConfig::MQTT_CONNECT_TIMEOUT_MS);

uint32_t lastPublishAt = 0;

void attachPwm(uint8_t pin, uint8_t channel) {
#if ESP_ARDUINO_VERSION_MAJOR >= 3
  ledcAttach(pin, AppConfig::PWM_FREQUENCY, AppConfig::PWM_RESOLUTION);
#else
  ledcSetup(channel, AppConfig::PWM_FREQUENCY, AppConfig::PWM_RESOLUTION);
  ledcAttachPin(pin, channel);
#endif
}

void writePwm(uint8_t pin, uint8_t channel, uint8_t value) {
#if ESP_ARDUINO_VERSION_MAJOR >= 3
  ledcWrite(pin, value);
#else
  ledcWrite(channel, value);
#endif
}

void beginTrafficLight() {
  attachPwm(AppConfig::RED_LED_PIN, RED_PWM_CHANNEL);
  attachPwm(AppConfig::YELLOW_LED_PIN, YELLOW_PWM_CHANNEL);
  attachPwm(AppConfig::GREEN_LED_PIN, GREEN_PWM_CHANNEL);

  writePwm(AppConfig::RED_LED_PIN, RED_PWM_CHANNEL, 0);
  writePwm(AppConfig::YELLOW_LED_PIN, YELLOW_PWM_CHANNEL, 0);
  writePwm(AppConfig::GREEN_LED_PIN, GREEN_PWM_CHANNEL, 0);
}

bool parseState(JsonVariantConst value, bool& enabled) {
  if (value.is<bool>()) {
    enabled = value.as<bool>();
    return true;
  }

  if (!value.is<const char*>()) {
    return false;
  }

  const char* state = value.as<const char*>();

  if (strcasecmp(state, "ON") == 0) {
    enabled = true;
    return true;
  }

  if (strcasecmp(state, "OFF") == 0) {
    enabled = false;
    return true;
  }

  return false;
}

bool applyTrafficLightCommand(const char* led, bool enabled, int pwm) {
  if (led == nullptr || led[0] == '\0') {
    return false;
  }

  const uint8_t output = enabled
                             ? static_cast<uint8_t>(constrain(pwm, 0, 255))
                             : 0;

  if (strcasecmp(led, "red") == 0) {
    writePwm(AppConfig::RED_LED_PIN, RED_PWM_CHANNEL,
             output);
  } else if (strcasecmp(led, "yellow") == 0) {
    writePwm(AppConfig::YELLOW_LED_PIN, YELLOW_PWM_CHANNEL,
             output);
  } else if (strcasecmp(led, "green") == 0) {
    writePwm(AppConfig::GREEN_LED_PIN, GREEN_PWM_CHANNEL,
             output);
  } else {
    return false;
  }

  return true;
}

void handleActuatorMessage(const char* topic, const char* payload) {
  if (topic == nullptr || payload == nullptr ||
      strcmp(topic, AppConfig::MQTT_ACTUATOR_TOPIC) != 0) {
    return;
  }

  JsonDocument document;
  const DeserializationError error = deserializeJson(document, payload);

  if (error || !document.is<JsonObject>()) {
    if (LOG_REJECTED_COMMANDS) {
      Serial.printf("[ACTUATOR] Ignored payload: %s\n", payload);
    }
    return;
  }

  if (!document["led"].is<const char*>()) {
    if (LOG_REJECTED_COMMANDS) {
      Serial.println("[ACTUATOR] Ignored: missing LED");
    }
    return;
  }

  const char* led = document["led"].as<const char*>();
  bool enabled = false;

  if (!parseState(document["state"], enabled)) {
    if (LOG_REJECTED_COMMANDS) {
      Serial.println("[ACTUATOR] Ignored: invalid state");
    }
    return;
  }

  int pwm = 255;

  if (!document["pwm"].isNull()) {
    if (!document["pwm"].is<int>() && !document["pwm"].is<float>()) {
      if (LOG_REJECTED_COMMANDS) {
        Serial.println("[ACTUATOR] Ignored: invalid PWM");
      }
      return;
    }

    pwm = document["pwm"].as<int>();
  }

  if (!applyTrafficLightCommand(led, enabled, pwm)) {
    if (LOG_REJECTED_COMMANDS) {
      Serial.println("[ACTUATOR] Ignored: unknown LED");
    }
    return;
  }

  Serial.printf("[ACTUATOR] LED: %s | State: %s | PWM: %d\n",
                led,
                enabled ? "ON" : "OFF",
                enabled ? constrain(pwm, 0, 255) : 0);
}

bool createSensorPayload(char* payload,
                         size_t payloadCapacity,
                         size_t& payloadLength) {
  JsonDocument document;
  document["temperature"] = random(2000, 3501) / 100.0F;
  document["humidity"] = random(4000, 8001) / 100.0F;

  if (measureJson(document) + 1 > payloadCapacity) {
    return false;
  }

  payloadLength = serializeJson(document, payload, payloadCapacity);
  return payloadLength > 0;
}

void publishSensorData() {
  char payload[128];
  size_t payloadLength = 0;

  if (!createSensorPayload(payload, sizeof(payload), payloadLength)) {
    Serial.println("[MQTT] Sensor payload overflow");
    return;
  }

  const uint16_t packetId = mqttManager.publish(
      AppConfig::MQTT_SENSOR_TOPIC, payload, payloadLength, 0, false);

  if (packetId == 0) {
    Serial.println("[MQTT] Publish failed");
    return;
  }

  Serial.printf("[MQTT] TX %s: %s\n",
                AppConfig::MQTT_SENSOR_TOPIC, payload);
}

void setup() {
  Serial.begin(115200);
  randomSeed(esp_random());

  Serial.printf("[APP] Mode: %s\n",
                APP_MODE == AppMode::ACTUATOR_ONLY
                    ? "ACTUATOR_ONLY"
                    : "SENSOR_AND_ACTUATOR");

  beginTrafficLight();
  mqttManager.begin(handleActuatorMessage);
  wifiManager.begin();
}

void loop() {
  wifiManager.update();
  mqttManager.update(wifiManager.isConnected());

  const uint32_t now = millis();

  if (APP_MODE == AppMode::SENSOR_AND_ACTUATOR &&
      mqttManager.isConnected() &&
      now - lastPublishAt >= AppConfig::SENSOR_PUBLISH_INTERVAL_MS) {
    lastPublishAt = now;
    publishSensorData();
  }
}
