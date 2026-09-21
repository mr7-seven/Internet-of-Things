#pragma once

#include <Arduino.h>

#ifndef PARKING_TOPOLOGY
#define PARKING_TOPOLOGY 2
#endif

#ifndef PARKING_NODE_ID
#define PARKING_NODE_ID 1
#endif

// Cukup ubah ID ini untuk setiap kelompok.
#define PARKING_GROUP_ID "kelompok01"

#define PARKING_STRINGIFY_INNER(value) #value
#define PARKING_STRINGIFY(value) PARKING_STRINGIFY_INNER(value)

namespace AppConfig {

// Wi-Fi
inline constexpr char WIFI_SSID[] = "NAMA_WIFI";
inline constexpr char WIFI_PASSWORD[] = "PASSWORD_WIFI";

// Identitas kelompok
inline constexpr char GROUP_ID[] = PARKING_GROUP_ID;

// MQTT broker
inline constexpr char MQTT_HOST[] = "broker.hivemq.com";
inline constexpr uint16_t MQTT_PORT = 1883;

// Node-RED mengirim perintah aktuator ke topik ini.
inline constexpr char MQTT_COMMAND_TOPIC[] =
    "parking/"
    PARKING_GROUP_ID
    "/system/command";

// ESP32 mengirim event entry atau exit ke topik ini.
inline constexpr char MQTT_EVENT_TOPIC[] =
    "parking/"
    PARKING_GROUP_ID
    "/node"
    PARKING_STRINGIFY(PARKING_NODE_ID)
    "/event";

// ESP32 mengirim status online ke topik ini.
inline constexpr char MQTT_STATUS_TOPIC[] =
    "parking/"
    PARKING_GROUP_ID
    "/node"
    PARKING_STRINGIFY(PARKING_NODE_ID)
    "/status";

// Last Will dikirim broker ketika ESP32 terputus.
inline constexpr char MQTT_OFFLINE_PAYLOAD[] =
    "{"
    "\"online\":false,"
    "\"group_id\":\"" PARKING_GROUP_ID "\","
    "\"node_id\":\"node"
    PARKING_STRINGIFY(PARKING_NODE_ID)
    "\","
    "\"topology\":"
    PARKING_STRINGIFY(PARKING_TOPOLOGY)
    "}";

// Pin perangkat
inline constexpr uint8_t RED_LED_PIN = 25;
inline constexpr uint8_t YELLOW_LED_PIN = 26;
inline constexpr uint8_t GREEN_LED_PIN = 27;
inline constexpr uint8_t IR_SENSOR_PIN = 5;
inline constexpr uint8_t EXIT_BUTTON_PIN = 33;
inline constexpr uint8_t BUZZER_PIN = 32;

// Konfigurasi input
inline constexpr bool IR_ACTIVE_LOW = true;
inline constexpr uint8_t IR_ACTIVE_LEVEL =
    IR_ACTIVE_LOW ? LOW : HIGH;

inline constexpr uint8_t BUTTON_ACTIVE_LEVEL = LOW;

// Interval aplikasi
inline constexpr uint32_t INPUT_DEBOUNCE_MS = 50;
inline constexpr uint32_t IR_EVENT_COOLDOWN_MS = 1500;
inline constexpr uint32_t BUZZER_BLINK_INTERVAL_MS = 300;
inline constexpr uint32_t STATUS_INTERVAL_MS = 10000;

// Wi-Fi reconnect
inline constexpr uint32_t WIFI_RECONNECT_INTERVAL_MS = 5000;
inline constexpr uint32_t WIFI_CONNECT_TIMEOUT_MS = 15000;

// MQTT reconnect
inline constexpr uint32_t MQTT_RECONNECT_INTERVAL_MS = 5000;
inline constexpr uint32_t MQTT_CONNECT_TIMEOUT_MS = 10000;

}  // namespace AppConfig