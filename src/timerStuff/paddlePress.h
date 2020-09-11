#ifndef PADDLEPRESS_H
#define PADDLEPRESS_H
#include <Arduino.h>

enum DitOrDah
{
    DIT,
    DAH,
    DUMMY
};

class PaddlePressDetection
{
public:
    PaddlePressDetection()
    {
        this->TimeStamp = millis();
    }
    DitOrDah Detected;
    long TimeStamp;
};
#endif