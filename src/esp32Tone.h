#include "toneBase.h"

class esp32Tone : public toneBase
{
public:
    virtual void noTone(uint8_t pin){};
    virtual void tone(uint8_t pin, unsigned short freq, unsigned duration = 0){};
};