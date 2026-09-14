#include <Arduino.h>
#include <esp_arduino_version.h>

constexpr uint8_t LED_PIN = 25;
constexpr uint8_t PWM_CHANNEL = 0;
constexpr uint32_t PWM_FREQUENCY = 5000;
constexpr uint8_t PWM_RESOLUTION = 8;

constexpr uint32_t PWM_INTERVAL = 10;

uint32_t previousMillis = 0;
int16_t pwmValue = 0;
int8_t pwmDirection = 1;

void setupPwm()
{
#if ESP_ARDUINO_VERSION_MAJOR >= 3
    ledcAttach(LED_PIN, PWM_FREQUENCY, PWM_RESOLUTION);
#else
    ledcSetup(PWM_CHANNEL, PWM_FREQUENCY, PWM_RESOLUTION);
    ledcAttachPin(LED_PIN, PWM_CHANNEL);
#endif
}

void writePwm(uint8_t value)
{
#if ESP_ARDUINO_VERSION_MAJOR >= 3
    ledcWrite(LED_PIN, value);
#else
    ledcWrite(PWM_CHANNEL, value);
#endif
}

void setup()
{
    Serial.begin(115200);

    setupPwm();
    writePwm(0);
}

void loop()
{
    uint32_t currentMillis = millis();

    if (currentMillis - previousMillis >= PWM_INTERVAL)
    {
        previousMillis = currentMillis;

        pwmValue += pwmDirection;

        if (pwmValue >= 255)
        {
            pwmValue = 255;
            pwmDirection = -1;
        }
        else if (pwmValue <= 0)
        {
            pwmValue = 0;
            pwmDirection = 1;
        }

        writePwm(static_cast<uint8_t>(pwmValue));
    }
}