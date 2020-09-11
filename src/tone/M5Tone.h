#ifndef M5TONE_H
#define M5TONE_H
#include "toneBase.h"
#include <M5Stack.h>
class M5Tone : public toneBase
{
public:
    void initialize()
    {
        M5.Speaker.begin();
    }
    M5Tone();
    virtual void noTone();
    virtual void tone(unsigned short freq, unsigned duration = 0);
};
#endif