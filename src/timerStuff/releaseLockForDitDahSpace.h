#ifndef RELEASELOCKFORDITDAHSPACE_H
#define RELEASELOCKFORDITDAHSPACE_H
#include <timerStuff/timerStuff.h>

// fired by timer holds sound during dits & dah spacing
void IRAM_ATTR releaseLockForDitDahSpace()
{

    soundPlaying = false;
    //ISR_Timer.disable(ditDahSpaceLockTimer);
}
#endif