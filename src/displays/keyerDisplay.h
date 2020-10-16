#ifndef KEYERDISPLAY_H
#define KEYERDISPLAY_H
#include "keyer_esp32.h"
#include "displays/displayCache.h"

#if defined M5DISPLAY
#include "displays/M5KeyerDisplay.h"
M5KeyerDisplay disp;
#elif defined OLED64X128
#include "displays/oled64x128.h"
oled64x128 disp;
#endif

displayBase &displayControl{disp};

DisplayCache displayCache;

#endif