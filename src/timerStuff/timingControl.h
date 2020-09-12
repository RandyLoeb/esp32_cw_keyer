#ifndef TIMINGCONTROL_H
#define TIMINGCONTROL_H
#include <Arduino.h>
#include <string>
class TimingControl
{

public:
    int wpm;
    int dit_ms;
    int dah_ms;
    int wordSpace_ms;
    int intraCharSpace_ms;
    int interCharSpace_ms;
    int dit_debounce_ms;
    int dah_debounce_ms;
    double secondsPerWord;
    double wordsPerSecond;
    double ditsPerWord;
    double secondsPerDit;

    void setWpm(int wpm)
    {
        this->wordsPerSecond = ((double)wpm) / ((double)60);
        this->secondsPerWord = 1 / this->wordsPerSecond;
        this->ditsPerWord = 50; //based on PARIS standard
        this->secondsPerDit = (1 / this->ditsPerWord) * this->secondsPerWord;
        this->dit_ms = this->secondsPerDit * 1000;
        this->dah_ms = this->dit_ms * 3;
        this->intraCharSpace_ms = this->dit_ms;
        this->wordSpace_ms = this->dit_ms * 7;
        this->interCharSpace_ms = this->dit_ms * 3;

        Serial.print("dit_ms:" + String(this->dit_ms) + " ");
        Serial.print("ddah_ms:" + String(this->dah_ms) + " ");
        Serial.print("intra_ms:" + String(this->intraCharSpace_ms) + " ");
        Serial.print("inter_ms:" + String(this->interCharSpace_ms) + " ");
        Serial.println("wordspace_ms:" + String(this->wordSpace_ms) + " ");
        }
};

TimingControl timingControl;
#endif