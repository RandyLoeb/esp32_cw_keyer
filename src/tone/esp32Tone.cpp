#include "esp32Tone.h"
#define SOUND_PWM_CHANNEL 0
#define SOUND_RESOLUTION 8                     // 8 bit resolution
#define SOUND_ON (1 << (SOUND_RESOLUTION - 1)) // 50% duty cycle
#define SOUND_OFF 0                            // 0% duty cycle

esp32Tone::esp32Tone(uint8_t pin)
{
    this->pin = pin;
}

void esp32Tone::noTone()
{
    this->tone(pin, -1);
}
void esp32Tone::tone(unsigned short freq,
                     unsigned duration)
{
    /* Serial.print("Tone:");
    Serial.print(freq);
    Serial.print(" Duration:");
    Serial.print(duration);
    Serial.print(" Pin:");
    Serial.println(pin); */

    ledcSetup(SOUND_PWM_CHANNEL, freq, SOUND_RESOLUTION); // Set up PWM channel
    ledcAttachPin(pin, SOUND_PWM_CHANNEL);
    ledcWriteTone(SOUND_PWM_CHANNEL, freq); // Attach channel to pin

    delay(duration);
}
