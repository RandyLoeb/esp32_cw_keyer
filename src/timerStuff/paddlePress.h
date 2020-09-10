#include <Arduino.h>

enum DitOrDah
{
    DIT,
    DAH
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