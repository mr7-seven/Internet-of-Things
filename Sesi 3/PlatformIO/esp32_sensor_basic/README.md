# Program Dasar Sensor ESP32

Paket ini berisi dua proyek PlatformIO yang berdiri sendiri.

## DHT11 atau DHT22

Folder: `esp32_dht_basic`

- Pin data: GPIO 5
- DHT11: interval pembacaan 1 detik
- DHT22: interval pembacaan 2 detik

Jenis sensor dipilih pada `src/main.cpp`:

```cpp
#define DHT_SENSOR_TYPE DHT11
```

Untuk DHT22, ubah menjadi:

```cpp
#define DHT_SENSOR_TYPE DHT22
```

## SRF-04

Folder: `esp32_srf04_basic`

- Trigger: GPIO 25
- Echo: GPIO 26
- Interval pembacaan: 1 detik
- Timeout Echo: 30 ms

Pin Echo SRF-04 harus dihubungkan ke ESP32 melalui pembagi tegangan karena
keluaran Echo 5 V, sedangkan GPIO ESP32 menggunakan logika 3,3 V.

## Perintah PlatformIO CLI

Jalankan perintah dari dalam salah satu folder proyek:

```bash
pio run
pio run --target upload
pio device monitor
```
