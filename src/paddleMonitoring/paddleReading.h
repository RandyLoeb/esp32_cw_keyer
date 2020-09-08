#ifndef PADDLEREADING_H
#define PADDLEREADING_H
#include <Arduino.h>
class PaddleReading
{

public:
    bool ditPaddlePressed = false;
    bool dahPaddlePressed = false;
    long startMillis = 0;
};
#endif