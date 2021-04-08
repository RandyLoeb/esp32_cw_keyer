#ifndef HARDWAREDITDAHHANDLERS_H
#define HARDWAREDITDAHHANDLERS_H
#include <timerStuff/timerStuff.h>

// these two are triggered by hardware interrupts
void IRAM_ATTR detectDitP(bool calledFromInterrupt, bool calledFromLocker, bool calledFromIambic)
{
    //Serial.print("detectDitPress called from locker:");
    //Serial.println(calledFromLocker);
    detectPress(&ditLocked, &ditPressed, ditTimer, dahTimer, debounceDitTimer, VIRTUAL_DITS, DitOrDah::DIT, calledFromInterrupt, calledFromLocker, calledFromIambic, &ditReleaseCache, &dahReleaseCache);
}

void IRAM_ATTR detectDahP(bool calledFromInterrupt, bool calledFromLocker, bool calledFromIambic)
{
    //Serial.print("detectDahPress called from locker:");
    //Serial.println(calledFromLocker);
    detectPress(&dahLocked, &dahPressed, dahTimer, ditTimer, debounceDahTimer, VIRTUAL_DAHS, DitOrDah::DAH, calledFromInterrupt, calledFromLocker, calledFromIambic, &dahReleaseCache, &ditReleaseCache);
}

void IRAM_ATTR detectDitPress()
{
    //Serial.println("dit change");
    detectDitP(true, false, false);

    //detectPress(&ditLocked, &ditPressed, ditTimer, dahTimer, debounceDitTimer, VIRTUAL_DITS, DitOrDah::DIT);
}

void IRAM_ATTR detectDahPress()
{
    //Serial.println("dah change");
    detectDahP(true, false, false);
    //detectPress(&dahLocked, &dahPressed, dahTimer, ditTimer, debounceDahTimer, VIRTUAL_DAHS, DitOrDah::DAH);
}

#endif