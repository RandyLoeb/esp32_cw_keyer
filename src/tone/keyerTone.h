#ifndef KEYERTONE_H
#define KEYERTONE_H
#include "keyer_esp32.h"
// Different hardware might need different tone control capabilities,
// so the approach is to use a global object that implements
// .tone and .noTone.

#include "toneCollection.h"
ToneCollection toneCollection;

#if defined M5CORE
#include "M5Tone.h"
M5Tone m5Tone;
#endif

#include "esp32pinTone.h"

#if defined M5CORE
// pins known to work on m5 = 26,25,17,16
// not work on m5 = 35, 36,3,1 (works but screws up serial printlns?),
ESP32PINTONE pinTone(GPIO_NUM_25);
#endif

#if defined REMOTE_KEYER
ESP32PINTONE pinTone(GPIO_NUM_22);
#endif

toneBase &toneControl{toneCollection};

void initializeTone()
{
#if defined M5CORE
    toneCollection.ToneControls.push_back(&m5Tone);
#endif

    toneCollection.ToneControls.push_back(&pinTone);
    toneCollection.initialize();
}
#endif