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

enum PaddlePressSource
{
    PADDLES,
    ARTIFICIAL
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
        this->Source = PADDLES;
    }
    PaddlePressSource Source;
    DitOrDah Detected;
    long TimeStamp;
    bool Display;
    bool Transmit;
    bool SideTone;
};
#endif