#include <Arduino.h>
#include <DHT.h>

constexpr uint8_t DHT_PIN = 5;

#define DHT_SENSOR_TYPE DHT11

#if DHT_SENSOR_TYPE == DHT22
constexpr uint32_t READ_INTERVAL_MS = 2000;
#else
constexpr uint32_t READ_INTERVAL_MS = 1000;
#endif

DHT dht(DHT_PIN, DHT_SENSOR_TYPE);
uint32_t lastReadAt = 0;

void readDhtSensor() {
  const float humidity = dht.readHumidity();
  const float temperature = dht.readTemperature();

  if (isnan(temperature) || isnan(humidity)) {
    Serial.println("[DHT] Gagal membaca sensor");
    return;
  }

  Serial.printf("[DHT] Temperatur: %.2f C | Kelembapan: %.2f %%\n",
                temperature, humidity);
}

void setup() {
  Serial.begin(115200);
  dht.begin();
}

void loop() {
  const uint32_t now = millis();

  if (now - lastReadAt >= READ_INTERVAL_MS) {
    lastReadAt = now;
    readDhtSensor();
  }
}

