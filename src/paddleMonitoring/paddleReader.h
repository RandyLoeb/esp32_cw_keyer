#ifndef PADDLEREADER_H
#define PADDLEREADER_H
#include <Arduino.h>
#include "paddleReading.h"
class PaddleReader
{

    PaddleReading *lastReadingState;
    int _wpm;
    const int _ditsPerWord = 50; //based on PARIS examples
    long _millisecondsPerDit;

    bool analyzeTimePassage(int ditsPassed, bool ditPressed, bool dahPressed);

    enum injections
    {
        dit,
        dah,
        char_space,
        word_space
    };

    injections lastInjection = word_space;

    void inject(injections injection);

    int (*_readDitPaddle)();
    int (*_readDahPaddle)();

public:
    PaddleReader(int (*readDitPaddle)(), int (*readDahPaddle)(), int wpm);

    void setWpm(int wpm);

    void readPaddles();
};
#endif