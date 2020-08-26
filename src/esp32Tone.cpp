#include "toneBase.h"
#define SOUND_PWM_CHANNEL 0
#define SOUND_RESOLUTION 8                     // 8 bit resolution
#define SOUND_ON (1 << (SOUND_RESOLUTION - 1)) // 50% duty cycle
#define SOUND_OFF 0                            // 0% duty cycle
class esp32Tone : public toneBase
{
public:
    virtual void noTone(uint8_t pin) 
    {
        this->tone(pin, -1);
    };
    virtual void tone(uint8_t pin, unsigned short freq, unsigned duration = 0) 
    {
        ledcSetup(SOUND_PWM_CHANNEL, freq, SOUND_RESOLUTION); // Set up PWM channel
        ledcAttachPin(pin, SOUND_PWM_CHANNEL);
        ledcWriteTone(SOUND_PWM_CHANNEL, freq); // Attach channel to pin
        //ledcWrite(SOUND_PWM_CHANNEL, SOUND_ON);

        delay(duration);
        //ledcWrite(SOUND_PWM_CHANNEL, SOUND_OFF);
    };
};