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

ESP32PINTONE pinTone(TONE_PIN);

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