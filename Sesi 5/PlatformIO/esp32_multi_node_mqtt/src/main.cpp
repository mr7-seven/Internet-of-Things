#include <Arduino.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <cstring>

#include "app_config.h"
#include "mqtt_manager.h"
#include "wifi_manager.h"

#if PARKING_TOPOLOGY != 2 && PARKING_TOPOLOGY != 3
#error "PARKING_TOPOLOGY harus 2 atau 3"
#endif

#if PARKING_NODE_ID < 1 || PARKING_NODE_ID > PARKING_TOPOLOGY
#error "PARKING_NODE_ID tidak sesuai dengan PARKING_TOPOLOGY"
#endif

namespace {
constexpr bool HAS_IR = PARKING_NODE_ID == 1;
constexpr bool HAS_TRAFFIC =
    (PARKING_TOPOLOGY == 2 && PARKING_NODE_ID == 1) ||
    (PARKING_TOPOLOGY == 3 && PARKING_NODE_ID == 2);
constexpr bool HAS_BUTTON =
    (PARKING_TOPOLOGY == 2 && PARKING_NODE_ID == 2) ||
    (PARKING_TOPOLOGY == 3 && PARKING_NODE_ID == 3);
constexpr bool HAS_BUZZER =
    (PARKING_TOPOLOGY == 2 && PARKING_NODE_ID == 2) ||
    (PARKING_TOPOLOGY == 3 && PARKING_NODE_ID == 3);

struct DebouncedInput {
  uint8_t pin;
  uint8_t activeLevel;
  int rawState;
  int stableState;
  uint32_t changedAt;
};

DebouncedInput irInput{AppConfig::IR_SENSOR_PIN,
                       AppConfig::IR_ACTIVE_LEVEL,
                       HIGH,
                       HIGH,
                       0};
DebouncedInput buttonInput{AppConfig::EXIT_BUTTON_PIN,
                           AppConfig::BUTTON_ACTIVE_LEVEL,
                           HIGH,
                           HIGH,
                           0};

uint32_t lastIrEventAt = 0;
uint32_t nextStatusAt = 0;
uint32_t nextBuzzerToggleAt = 0;
uint32_t eventSequence = 0;
bool buzzerBlink = false;
bool buzzerOutput = false;
bool mqttWasConnected = false;
}

WifiManager wifiManager(AppConfig::WIFI_SSID,
                        AppConfig::WIFI_PASSWORD,
                        AppConfig::WIFI_RECONNECT_INTERVAL_MS,
                        AppConfig::WIFI_CONNECT_TIMEOUT_MS);

MqttManager mqttManager(AppConfig::MQTT_HOST,
                        AppConfig::MQTT_PORT,
                        AppConfig::MQTT_COMMAND_TOPIC,
                        AppConfig::MQTT_STATUS_TOPIC,
                        AppConfig::MQTT_OFFLINE_PAYLOAD,
                        AppConfig::MQTT_RECONNECT_INTERVAL_MS,
                        AppConfig::MQTT_CONNECT_TIMEOUT_MS);

bool updateInput(DebouncedInput& input, uint32_t now) {
  const int reading = digitalRead(input.pin);

  if (reading != input.rawState) {
    input.rawState = reading;
    input.changedAt = now;
  }

  if (reading != input.stableState &&
      now - input.changedAt >= AppConfig::INPUT_DEBOUNCE_MS) {
    input.stableState = reading;
    return input.stableState == input.activeLevel;
  }

  return false;
}

void setTrafficSignal(const char* signal) {
  if (!HAS_TRAFFIC) return;

  const bool red = strcmp(signal, "red") == 0;
  const bool yellow = strcmp(signal, "yellow") == 0;
  const bool green = strcmp(signal, "green") == 0;

  if (!red && !yellow && !green) return;

  digitalWrite(AppConfig::RED_LED_PIN, red ? HIGH : LOW);
  digitalWrite(AppConfig::YELLOW_LED_PIN, yellow ? HIGH : LOW);
  digitalWrite(AppConfig::GREEN_LED_PIN, green ? HIGH : LOW);
  Serial.printf("[SIGNAL] %s\n", signal);
}

void setBuzzerMode(const char* mode) {
  if (!HAS_BUZZER) return;

  buzzerBlink = strcmp(mode, "blink") == 0;

  if (!buzzerBlink) {
    buzzerOutput = false;
    digitalWrite(AppConfig::BUZZER_PIN, LOW);
  }

  nextBuzzerToggleAt = millis();
  Serial.printf("[BUZZER] %s\n", buzzerBlink ? "BLINK" : "OFF");
}

void publishEvent(const char* eventName) {
  if (!mqttManager.isConnected()) return;

  JsonDocument document;
  char payload[192];
  char eventId[80];
  char nodeId[12];

  snprintf(nodeId, sizeof(nodeId), "node%u", PARKING_NODE_ID);

  snprintf(eventId,
         sizeof(eventId),
         "%s-n%u-%lu-%lu",
         AppConfig::GROUP_ID,
         PARKING_NODE_ID,
         static_cast<unsigned long>(millis()),
         static_cast<unsigned long>(++eventSequence));

  document["group_id"] = AppConfig::GROUP_ID;
  document["event"] = eventName;
  document["event_id"] = eventId;
  document["node_id"] = nodeId;
  document["topology"] = PARKING_TOPOLOGY;

  const size_t length = serializeJson(document, payload, sizeof(payload));
  mqttManager.publish(AppConfig::MQTT_EVENT_TOPIC, payload, length, 1, false);
  Serial.printf("[EVENT] %s | %s\n", eventName, eventId);
}

void publishStatus() {
  if (!mqttManager.isConnected()) return;

  JsonDocument document;
  char payload[192];
  char nodeId[12];

  snprintf(nodeId, sizeof(nodeId), "node%u", PARKING_NODE_ID);

  document["online"] = true;
  document["group_id"] = AppConfig::GROUP_ID;
  document["node_id"] = nodeId;
  document["topology"] = PARKING_TOPOLOGY;
  document["uptime_ms"] = millis();
  document["rssi"] = WiFi.RSSI();

  const size_t length = serializeJson(document, payload, sizeof(payload));
  mqttManager.publish(AppConfig::MQTT_STATUS_TOPIC, payload, length, 1, true);
}

void handleCommand(const char* topic, const char* payload) {
  if (topic == nullptr || payload == nullptr ||
      strcmp(topic, AppConfig::MQTT_COMMAND_TOPIC) != 0) {
    return;
  }

  JsonDocument document;
  if (deserializeJson(document, payload) || !document.is<JsonObject>()) return;

  const char* signal = document["signal"] | "";
  const char* buzzer = document["buzzer"] | "off";

  setTrafficSignal(signal);
  setBuzzerMode(buzzer);
}

void updateBuzzer(uint32_t now) {
  if (!HAS_BUZZER || !buzzerBlink ||
      static_cast<int32_t>(now - nextBuzzerToggleAt) < 0) {
    return;
  }

  buzzerOutput = !buzzerOutput;
  digitalWrite(AppConfig::BUZZER_PIN, buzzerOutput ? HIGH : LOW);
  nextBuzzerToggleAt = now + AppConfig::BUZZER_BLINK_INTERVAL_MS;
}

void setup() {
  Serial.begin(115200);

  if (HAS_IR) {
    pinMode(AppConfig::IR_SENSOR_PIN, INPUT_PULLUP);
    irInput.rawState = irInput.stableState = digitalRead(irInput.pin);
  }

  if (HAS_BUTTON) {
    pinMode(AppConfig::EXIT_BUTTON_PIN, INPUT_PULLUP);
    buttonInput.rawState = buttonInput.stableState = digitalRead(buttonInput.pin);
  }

  if (HAS_TRAFFIC) {
    pinMode(AppConfig::RED_LED_PIN, OUTPUT);
    pinMode(AppConfig::YELLOW_LED_PIN, OUTPUT);
    pinMode(AppConfig::GREEN_LED_PIN, OUTPUT);
    setTrafficSignal("green");
  }

  if (HAS_BUZZER) {
    pinMode(AppConfig::BUZZER_PIN, OUTPUT);
    digitalWrite(AppConfig::BUZZER_PIN, LOW);
  }

  mqttManager.begin(handleCommand);
  wifiManager.begin();

  Serial.printf("[APP] Parking topology %u | node %u\n",
                PARKING_TOPOLOGY,
                PARKING_NODE_ID);
}

void loop() {
  const uint32_t now = millis();

  wifiManager.update();
  mqttManager.update(wifiManager.isConnected());

  const bool mqttConnected = mqttManager.isConnected();
  if (mqttConnected && !mqttWasConnected) {
    publishStatus();
    nextStatusAt = now + AppConfig::STATUS_INTERVAL_MS;
  }
  mqttWasConnected = mqttConnected;

  if (mqttConnected && static_cast<int32_t>(now - nextStatusAt) >= 0) {
    publishStatus();
    nextStatusAt = now + AppConfig::STATUS_INTERVAL_MS;
  }

  if (HAS_IR && updateInput(irInput, now) &&
      now - lastIrEventAt >= AppConfig::IR_EVENT_COOLDOWN_MS) {
    lastIrEventAt = now;
    publishEvent("entry");
  }

  if (HAS_BUTTON && updateInput(buttonInput, now)) {
    publishEvent("exit");
  }

  updateBuzzer(now);
}
