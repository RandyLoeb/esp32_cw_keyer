#ifndef PADDLEPRESS_H
#define PADDLEPRESS_H
#include <Arduino.h>

enum DitOrDah
{
    DIT,
    DAH,
    DUMMY,
    SPACE,
    FORCED_CHARSPACE
};

class PaddlePressDetection
{
public:
    PaddlePressDetection()
    {
        this->TimeStamp = millis();
        this->Display = true;
        this->Transmit = true;
        this->SideTone = true;
    }
    DitOrDah Detected;
    long TimeStamp;
    bool Display;
    bool Transmit;
    bool SideTone;
};
#endif