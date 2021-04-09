#ifndef CHANGETIMERWPM_H
#define CHANGETIMERWPM_H
#include <timerStuff/timerStuff.h>
void changeTimerWpm()
{

    timingControl.setWpm(configControl.configJsonDoc["wpm"].as<int>(), configControl.configJsonDoc["wpm_farnsworth"].as<int>(), configControl.configJsonDoc["wpm_farnsworth_slow"].as<int>());
    //ISR_Timer.disable(ditTimer);
    //ISR_Timer.disable(dahTimer);
    ISR_Timer.disable(toneSilenceTimer);
    //ISR_Timer.disable(ditDahSpaceLockTimer);
     ISR_Timer.disable(charSpaceTimer);
#if defined IAMBIC_ALTERNATE
    ISR_Timer.disable(iambicTimer);
#endif

    currentDitTiming = 1 + timingControl.Paddles.dit_ms + timingControl.Paddles.intraCharSpace_ms;
    currentDahTiming = 1 + timingControl.Paddles.dah_ms + timingControl.Paddles.intraCharSpace_ms;
    //ISR_Timer.changeInterval(ditTimer, currentDitTiming);
    //ISR_Timer.changeInterval(dahTimer, currentDahTiming);
    ISR_Timer.changeInterval(toneSilenceTimer, timingControl.Paddles.dit_ms);
    //ISR_Timer.changeInterval(ditDahSpaceLockTimer, timingControl.Paddles.intraCharSpace_ms);
    ISR_Timer.changeInterval(charSpaceTimer, 10 + timingControl.Paddles.dah_ms + timingControl.Paddles.intraCharSpace_ms);
#if defined IAMBIC_ALTERNATE
    baseIambicTiming = timingControl.Paddles.intraCharSpace_ms;
    ISR_Timer.changeInterval(iambicTimer, baseIambicTiming);
#endif

    
    ISR_Timer.disable(toneSilenceTimer);
    //ISR_Timer.disable(ditDahSpaceLockTimer);
    ISR_Timer.enable(charSpaceTimer);
#if defined IAMBIC_ALTERNATE
    ISR_Timer.disable(iambicTimer);
#endif
}
#endif