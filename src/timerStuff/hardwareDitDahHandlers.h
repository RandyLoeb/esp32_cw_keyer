#ifndef HARDWAREDITDAHHANDLERS_H
#define HARDWAREDITDAHHANDLERS_H
#include <timerStuff/timerStuff.h>

// these two are triggered by hardware interrupts
void IRAM_ATTR detectDitP(bool calledFromInterrupt, bool calledFromLocker, bool calledFromIambic)
{
    detectPress(&ditLocked, &ditPressed, debounceDitTimer, VIRTUAL_DITS, DitOrDah::DIT, calledFromInterrupt, calledFromLocker, calledFromIambic, &ditReleaseCache, &dahReleaseCache);
}

void IRAM_ATTR detectDahP(bool calledFromInterrupt, bool calledFromLocker, bool calledFromIambic)
{
    detectPress(&dahLocked, &dahPressed, debounceDahTimer, VIRTUAL_DAHS, DitOrDah::DAH, calledFromInterrupt, calledFromLocker, calledFromIambic, &dahReleaseCache, &ditReleaseCache);
}

void IRAM_ATTR detectDitPress()
{
    detectDitP(true, false, false);
}

void IRAM_ATTR detectDahPress()
{
    detectDahP(true, false, false);
}

#endif