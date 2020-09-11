#ifndef TONEBASE_H
#define TONEBASE_H
#include <Arduino.h>

class toneBase
{

protected:
    uint8_t pin;

public:
    virtual void initialize();
    virtual void noTone(){};
    virtual void tone(unsigned short freq,
                      unsigned duration = 0)
    {
        Serial.println("Sending tone from base...no good...");
    };
};
#endif