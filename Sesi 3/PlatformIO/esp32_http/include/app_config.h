#pragma once

#include <Arduino.h>

namespace AppConfig {
inline constexpr char WIFI_SSID[] = "NAMA_WIFI";
inline constexpr char WIFI_PASSWORD[] = "PASSWORD_WIFI";
inline constexpr char HTTP_URL[] = "http://192.168.1.100:1880/api/sensor";

inline constexpr uint32_t SEND_INTERVAL_MS = 10000;
inline constexpr uint32_t WIFI_RECONNECT_INTERVAL_MS = 5000;
inline constexpr uint32_t WIFI_CONNECT_TIMEOUT_MS = 15000;
inline constexpr uint32_t HTTP_TIMEOUT_MS = 5000;
}

