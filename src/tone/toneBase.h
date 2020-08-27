#include <Arduino.h>
class toneBase
{
public:
    virtual void noTone(uint8_t pin){};
    virtual void tone(uint8_t pin, unsigned short freq,
     unsigned duration = 0)
    {
        Serial.println("Sending tone from base...no good...");
    };
};