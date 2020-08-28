#include "toneBase.h"
#include <M5Stack.h>
class M5Tone : public toneBase
{
public:
    M5Tone();
    virtual void noTone();
    virtual void tone(unsigned short freq, unsigned duration = 0);
};