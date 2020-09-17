#ifndef KEYERDISPLAY_H
#define KEYERDISPLAY_H
#include "keyer_esp32.h"

#if defined M5CORE
#include "displays/M5KeyerDisplay.h"
M5KeyerDisplay disp;
#endif

#if defined REMOTE_KEYER
#include "displays/oled64x128.h"
oled64x128 disp;
#endif

displayBase &displayControl{disp};

#endif