#ifndef DEBOUNCERUNLOCKS_H
#define DEBOUNCERUNLOCKS_H
#include <timerStuff/timerStuff.h>
// fired by the timer that unlocks the debouncer indirectly
void IRAM_ATTR unlockDebouncer(void (*detectCallback)(bool, bool, bool), volatile bool *flagToFalse)
{
    *flagToFalse = false;

    //this is here so a release is seen in case it was swallowed by
    //a debounce lock
    detectCallback(false, true, false);
}

// fired by timer for unlocker direclty
void IRAM_ATTR unlockDit()
{
    unlockDebouncer(detectDitP, &ditLocked);
}

void IRAM_ATTR unlockDah()
{
    unlockDebouncer(detectDahP, &dahLocked);
}

#endif