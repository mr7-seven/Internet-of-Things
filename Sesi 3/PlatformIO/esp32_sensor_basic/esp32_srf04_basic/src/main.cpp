#include <Arduino.h>

constexpr uint8_t TRIGGER_PIN = 25;
constexpr uint8_t ECHO_PIN = 26;
constexpr uint32_t READ_INTERVAL_MS = 1000;
constexpr uint32_t ECHO_TIMEOUT_US = 30000;

uint32_t lastReadAt = 0;

float readDistanceCm() {
  digitalWrite(TRIGGER_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIGGER_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIGGER_PIN, LOW);

  const uint32_t durationUs = pulseIn(ECHO_PIN, HIGH, ECHO_TIMEOUT_US);

  if (durationUs == 0) {
    return NAN;
  }

  return durationUs * 0.0343F / 2.0F;
}

void setup() {
  Serial.begin(115200);

  pinMode(TRIGGER_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  digitalWrite(TRIGGER_PIN, LOW);
}

void loop() {
  const uint32_t now = millis();

  if (now - lastReadAt >= READ_INTERVAL_MS) {
    lastReadAt = now;

    const float distanceCm = readDistanceCm();

    if (isnan(distanceCm)) {
      Serial.println("[SRF-04] Pembacaan timeout");
      return;
    }

    Serial.printf("[SRF-04] Jarak: %.2f cm\n", distanceCm);
  }
}

