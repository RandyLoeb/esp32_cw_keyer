#include "M5Tone.h"
#include <M5Stack.h>

M5Tone::M5Tone()
{
    //M5.Speaker.begin();
}
void M5Tone::noTone()
{
    M5.Speaker.mute();
}
void M5Tone::tone(unsigned short freq,
                  unsigned duration)
{
    if (this->enabled)
    {
        M5.Speaker.tone(freq, duration);
    }
}
