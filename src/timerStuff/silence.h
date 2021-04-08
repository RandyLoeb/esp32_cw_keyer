#ifndef SILENCE_H
#define SILENCE_H
#include <timerStuff/timerStuff.h>

// fired by timer that ends a sidetone segment
void IRAM_ATTR silenceTone()
{

#if defined TRANSMIT
    transmitControl.unkey();
#endif

#if defined TONE_ON
    //M5.Speaker.mute();
    toneControl.noTone();
#endif
    // kickoff the spacing timer between dits & dahs
    ISR_Timer.disable(toneSilenceTimer);
    ISR_Timer.restartTimer(ditDahSpaceLockTimer);
    ISR_Timer.enable(ditDahSpaceLockTimer);
}

#endif