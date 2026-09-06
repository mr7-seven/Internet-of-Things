# ESP32 Wi-Fi and HTTP Template

Template ESP32 Arduino untuk koneksi Wi-Fi berbasis event, reconnect non-blocking,
dan pengiriman HTTP POST JSON menggunakan ArduinoJson v7. Konfigurasi platform
menggunakan ESP32 Arduino Core 3.x melalui platform PIOArduino.

## Konfigurasi

Ubah tiga nilai berikut pada `include/app_config.h`:

```cpp
inline constexpr char WIFI_SSID[] = "NAMA_WIFI";
inline constexpr char WIFI_PASSWORD[] = "PASSWORD_WIFI";
inline constexpr char HTTP_URL[] = "http://192.168.1.100:1880/api/sensor";
```

Payload contoh:

```json
{"temperature":27.35,"humidity":68.42}
```

## Perintah PlatformIO CLI

```bash
cd esp32_wifi_http_template
pio run
pio run --target upload
pio device monitor
```

Untuk mengganti sensor, ubah isi `createSensorPayload()` pada `src/main.cpp`.
Fungsi HTTP tetap dipanggil dengan pola berikut:

```cpp
httpManager.sendPayload(payload);
```

Kode `200` dianggap berhasil. Kode HTTP lain dan kegagalan koneksi ditampilkan
pada Serial Monitor.
