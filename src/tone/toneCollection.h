#ifndef TONECOLLECTION_H
#define TONECOLLECTION_H
#include "Arduino.h"
#include <vector>
#include "toneBase.h"
class ToneCollection : public toneBase
{
public:
    std::vector<toneBase *> ToneControls;
    void initialize()
    {
        for (std::vector<toneBase *>::iterator it = ToneControls.begin(); it != ToneControls.end(); ++it)
        {
            (*it)->initialize();
        }
    }

    void noTone()
    {
        for (std::vector<toneBase *>::iterator it = ToneControls.begin(); it != ToneControls.end(); ++it)
        {
            (*it)->noTone();
        }
    }

    void tone(unsigned short freq,
              unsigned duration = 0)
    {
        for (std::vector<toneBase *>::iterator it = ToneControls.begin(); it != ToneControls.end(); ++it)
        {
            (*it)->tone(freq, duration);
        }
    }
};

#endif