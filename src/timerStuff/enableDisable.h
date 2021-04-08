#ifndef ENABLEDISABLE_H
#define ENABLEDISABLE_H
#include <timerStuff/timerStuff.h>
void disableAllTimers()
{

    ISR_Timer.disableAll();
    //Serial.println("disabled all timers");
}

void reEnableTimers()
{
    //Serial.println("reenabling timers");
    ISR_Timer.enable(charSpaceTimer);

//not sure these need to be re-enabled, but maybe causing problem with slave that doesn't have pin interrupts?
#if !defined REMOTE_KEYER
    //Serial.println("re-enabling debouncers");
    ISR_Timer.enable(debounceDitTimer);
    ISR_Timer.enable(debounceDahTimer);
#endif
}
#endif