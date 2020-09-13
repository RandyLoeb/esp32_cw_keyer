#ifndef KEYERTONE_H
#define KEYERTONE_H
// Different hardware might need different tone control capabilities,
// so the approach is to use a global object that implements
// .tone and .noTone.
#include "toneCollection.h"
ToneCollection toneCollection;

#include "M5Tone.h"
M5Tone m5Tone;

#include "esp32pinTone.h"

// pins known to work on m5 = 26,25,17,16
// not work on m5 = 35, 36,3,1 (works but screws up serial printlns?),
ESP32PINTONE pinTone(GPIO_NUM_25);

toneBase &toneControl{toneCollection};

void initializeTone()
{
    toneCollection.ToneControls.push_back(&m5Tone);
    toneCollection.ToneControls.push_back(&pinTone);
    toneCollection.initialize();
}
#endif