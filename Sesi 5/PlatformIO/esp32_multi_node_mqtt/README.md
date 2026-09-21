# ESP32 Smart Parking Multi-Node

Satu source PlatformIO untuk topologi dua atau tiga ESP32. Wi-Fi dan MQTT
memakai manager non-blocking dari template sebelumnya.

## Pin

| Perangkat | GPIO |
|---|---:|
| LED merah | 25 |
| LED kuning | 26 |
| LED hijau | 27 |
| Sensor IR | 5 |
| Push button | 33 |
| Buzzer aktif | 32 |

GPIO buzzer dapat diganti melalui `include/app_config.h`.

## Pembagian node

### Topologi dua node

- Node 1: IR dan traffic light.
- Node 2: push button dan buzzer.

### Topologi tiga node

- Node 1: IR.
- Node 2: traffic light.
- Node 3: push button dan buzzer.

Sensor IR LM393 menggunakan logika aktif-LOW secara default melalui
`IR_ACTIVE_LOW = true` di `include/app_config.h`.

## Sinkronisasi MQTT

- ESP32 mengirim event unik ke `parking/nodeX/event` dengan QoS 1.
- ESP32 mengirim status retained ke `parking/nodeX/status`.
- Semua node menerima perintah aktuator dari `parking/system/command`.
- Perintah Node-RED dibuat retained agar aktuator yang reconnect langsung
  menerima status terakhir.

`AsyncTCP` dikunci ke versi `3.3.2` agar sesuai dengan
`AsyncMqttClient 0.9.0` dan tidak menghasilkan warning `close(bool)` deprecated.

## Build dan upload

```bash
pio run -e parking_2node_node1
pio run -e parking_2node_node2
pio run -e parking_3node_node1
pio run -e parking_3node_node2
pio run -e parking_3node_node3
```

Tambahkan `-t upload` untuk mengunggah environment yang dipilih.

## Payload ESP32

Kendaraan masuk:

```json
{"event":"entry","event_id":"n1-12000-1","node_id":"node1","topology":2}
```

Kendaraan keluar:

```json
{"event":"exit","event_id":"n2-15000-1","node_id":"node2","topology":2}
```

Heartbeat:

```json
{"online":true,"node_id":"node1","topology":2,"uptime_ms":20000,"rssi":-58}
```

## Payload dari Node-RED

```json
{"signal":"yellow","buzzer":"off","occupied":7}
```

Ketika parkir penuh:

```json
{"signal":"red","buzzer":"blink","occupied":10}
```

Ubah SSID, password, dan broker pada `include/app_config.h` sebelum upload.
