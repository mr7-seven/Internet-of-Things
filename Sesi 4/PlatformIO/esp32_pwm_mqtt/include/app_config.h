#pragma once

#include <Arduino.h>

#define DEVICE_NIM "12345"

namespace AppConfig {
inline constexpr char WIFI_SSID[] = "IoT";
inline constexpr char WIFI_PASSWORD[] = "12345678";

inline constexpr char MQTT_HOST[] = "broker.hivemq.com";
inline constexpr uint16_t MQTT_PORT = 1883;
inline constexpr char MQTT_SENSOR_TOPIC[] = "data/iot/sensor/" DEVICE_NIM;
inline constexpr char MQTT_ACTUATOR_TOPIC[] = "data/iot/actuator/" DEVICE_NIM;

inline constexpr uint32_t SENSOR_PUBLISH_INTERVAL_MS = 2000;
inline constexpr uint32_t WIFI_RECONNECT_INTERVAL_MS = 5000;
inline constexpr uint32_t WIFI_CONNECT_TIMEOUT_MS = 15000;
inline constexpr uint32_t MQTT_RECONNECT_INTERVAL_MS = 5000;
inline constexpr uint32_t MQTT_CONNECT_TIMEOUT_MS = 10000;

inline constexpr uint8_t RED_LED_PIN = 25;
inline constexpr uint8_t YELLOW_LED_PIN = 26;
inline constexpr uint8_t GREEN_LED_PIN = 27;
inline constexpr uint16_t PWM_FREQUENCY = 5000;
inline constexpr uint8_t PWM_RESOLUTION = 8;
}

