#ifndef INJECTCHARSPACE_H
#define INJECTCHARSPACE_H
#include <timerStuff/timerStuff.h>
void IRAM_ATTR injectCharSpace()
{
#if (TIMER_INTERRUPT_DEBUG > 0)
    //Serial.println("charspace");
#endif
    PaddlePressDetection *newPd = new PaddlePressDetection();
    newPd->Detected = DitOrDah::DUMMY;
    ditsNdahQueue.push(newPd);
    //ISR_Timer.disable(charSpaceTimer);
}
#endif