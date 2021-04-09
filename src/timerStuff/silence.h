#ifndef SILENCE_H
#define SILENCE_H
#include <timerStuff/timerStuff.h>

// fired by timer that ends a sidetone segment
void IRAM_ATTR silenceTone()
{
    if (!toneSilenceIntraLockMode)
    {
#if defined TRANSMIT
        transmitControl.unkey();
#endif

#if defined TONE_ON
        //M5.Speaker.mute();
        toneControl.noTone();
#endif

        // kickoff the spacing timer between dits & dahs
        toneSilenceIntraLockMode = 1;
        ISR_Timer.disable(toneSilenceTimer);
        ISR_Timer.changeInterval(toneSilenceTimer, timingControl.Paddles.intraCharSpace_ms);
        ISR_Timer.restartTimer(toneSilenceTimer);
        ISR_Timer.enable(toneSilenceTimer);
    }
    else
    {

        toneSilenceIntraLockMode = 0;
        soundPlaying = false;
        ISR_Timer.disable(toneSilenceTimer);
    }
}

#endif