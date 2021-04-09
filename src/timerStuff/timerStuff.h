/*
Had lots of problems trying to get some of the interrupts to accept class 
member functions (e.g. tried lambdas, etc) and wouldn't build. 
Is what it is for now
*/
#ifndef TIMERSTUFF_H
#define TIMERSTUFF_H
#include <Arduino.h>
#include <queue>
#include <string>
#include <M5Stack.h>
//These define's must be placed at the beginning before #include "ESP32TimerInterrupt.h"
#define TIMER_INTERRUPT_DEBUG 0
#include "ESP32TimerInterrupt.h"
#include "ESP32_ISR_Timer.h"
#include "conversionStuff/conversionStuff.h"
#include "paddlePress.h"
#include "virtualPins/keyerPins.h"
#include "tone/keyerTone.h"
#include "timingControl.h"
#include "webServer/webServer.h"
#include "keyer_esp32.h"
#include "keyer_esp32now.h"
#include "persistentConfig/persistentConfig.h"
#include "cwControl/cw_utils.h"

#if defined TRANSMIT
#include "transmitControl/transmitContol.h"
TransmitControl transmitControl;
#endif
CwControl *_cwControl;
persistentConfig *_timerStuffConfig;

#define IAMBIC_ALTERNATE

#include <timerStuff/webSocketsInit.h>

// queue to hold dits and dahs seen by the paddle monitorning,
// sound loop will pull from here
std::queue<PaddlePressDetection *> ditsNdahQueue;

volatile uint32_t lastMillis = 0;
// Init ESP32 timer 1
ESP32Timer ITimer(1);

// Init ESP32_ISR_Timer
ESP32_ISR_Timer ISR_Timer;

// this might be too low? 50 and 25 were too high for 20wpm
#define HW_TIMER_INTERVAL_MS 1

// sort of a a lockout
volatile bool soundPlaying = false;

// timer library seems to need this...
void IRAM_ATTR TimerHandler(void)
{
    ISR_Timer.run();
}

// holds our flags and timer handles
volatile bool detectInterrupts = false;
volatile bool ditPressed = false;
volatile bool dahPressed = false;
volatile int debounceDitTimer;
volatile int debounceDahTimer;
volatile int toneSilenceTimer;
volatile int toneSilenceIntraLockMode = 0;
//volatile int ditDahSpaceLockTimer;
volatile int charSpaceTimer;
#if defined IAMBIC_ALTERNATE
volatile int iambicTimer;
volatile bool iambicMode;
volatile DitOrDah lastInjection;
volatile long baseIambicTiming;

#endif
volatile long currentDitTiming;
volatile long currentDahTiming;
volatile bool ditLocked = false;
volatile bool dahLocked = false;
volatile int ditReleaseCache = 0;
volatile int dahReleaseCache = 0;
volatile int charSpaceInjects = 0;

void IRAM_ATTR ditDahInjector(DitOrDah dd)
{
#if defined IAMBIC_ALTERNATE
    lastInjection = dd;
#endif
    PaddlePressDetection *newPd = new PaddlePressDetection();
    newPd->Detected = dd;
    ditsNdahQueue.push(newPd);
}

// this is triggerd by hardware interrupt indirectly
void IRAM_ATTR detectPress(volatile bool *locker, volatile bool *pressed, int lockTimer, int pin, DitOrDah message, bool calledFromInterrupt, bool calledFromLocker, bool calledFromIambic, volatile int *releaseCache, volatile int *oppoReleaseCache)
{

    // locker is our debouce variable, i.e. we'll ignore any changes
    // to the pin during hte bounce
    if (!*locker)
    {
        if (!calledFromLocker)
        {
            // lock and kickoff the debouncer
            *locker = true;
            ISR_Timer.restartTimer(lockTimer);
            ISR_Timer.enable(lockTimer);
        }
        else
        {
            // also stop the lock timer
            ISR_Timer.disable(lockTimer);
        }

        // wait for interrupts to be turned on
        if (detectInterrupts)
        {

            // what was previous date of pin?
            int pressedBefore = *pressed;

            // get the pin
            *pressed = !virtualPins.pinsets[pin]->digRead();
            // pressed?

#if defined IAMBIC_ALTERNATE
            iambicMode = ditPressed && dahPressed;
            /* if (!iambicMode)
            {
                ISR_Timer.disable(iambicTimer);
            } */
#endif

            if (*pressed && !pressedBefore)

            {

                //for now iambic behavior is a press stops press of the other

                //04/08/21
                //ISR_Timer.disable(oppoTimer);

                ditDahInjector(message);

#if !defined IAMBIC_ALTERNATE
                //kickoff either the dit or dah timer passed in
                //so it will keep injecting into the queue

                ISR_Timer.restartTimer(timer);
                ISR_Timer.enable(timer);
#else
                //reset iambic timer to above injection length plus an intracharacter
                ISR_Timer.changeInterval(iambicTimer, message == DitOrDah::DIT ? currentDitTiming : currentDahTiming);
                ISR_Timer.restartTimer(iambicTimer);
                ISR_Timer.enable(iambicTimer);
#endif
            }
            // released?
            else if (!*pressed && pressedBefore)
            {
                // released so stop the timer

                //04/08/21
                //ISR_Timer.disable(timer);
            }
        }
    }
}

#include <timerStuff/hardwareDitDahHandlers.h>
#include <timerStuff/debouncerUnlocks.h>
#include <timerStuff/silence.h>
#include <timerStuff/releaseLockForDitDahSpace.h>
#include <timerStuff/injectCharSpace.h>

#if defined IAMBIC_ALTERNATE

//TODO: probably this can take over for both the dit and dah timers,
//      even in "single lever" mode, but not sure if mentally that is
//      easier to follow/maintain. something to think about.
void IRAM_ATTR iambicAction()
{
    //ISR_Timer.disable(iambicTimer);
    DitOrDah toInject;
    //in an iambic mode we arrive here after an intracharacter space
    //are we still in iambic mode?
    if (iambicMode)
    {
        charSpaceInjects = 0;
        //inject whomever's turn it is
        DitOrDah lastBeforeInjection = lastInjection;
        toInject = lastInjection == DitOrDah::DIT ? DitOrDah::DAH : DitOrDah::DIT;
        ditDahInjector(toInject);

        //reset iambic timer to above injection length plus an intracharacter
        ISR_Timer.changeInterval(iambicTimer, toInject == DitOrDah::DIT ? currentDitTiming : currentDahTiming);
        ISR_Timer.restartTimer(iambicTimer);
        ISR_Timer.enable(iambicTimer);
    }
    else
    {
        //turn off and handover to whichever paddle is left standing
        //we're here because we're not in iambic anymore, so see
        //if anything is pressed
        if (ditPressed || dahPressed)
        {
            charSpaceInjects = 0;
            //inject a survivor
            toInject = ditPressed ? DitOrDah::DIT : DitOrDah::DAH;
            ditDahInjector(toInject);

            //reset iambic timer to above injection length plus an intracharacter
            ISR_Timer.changeInterval(iambicTimer, toInject == DitOrDah::DIT ? currentDitTiming : currentDahTiming);
            ISR_Timer.restartTimer(iambicTimer);
            ISR_Timer.enable(iambicTimer);
        }
        else
        {
            ISR_Timer.disable(iambicTimer);

            /*
            if (charSpaceInjects > 0)
            {
                Serial.print(charSpaceInjects);
                Serial.print(" ");
                Serial.println("injecting space");
                //injectCharSpace();
                charSpaceInjects++;

                if (charSpaceInjects == 2)
                {
                    ISR_Timer.disable(iambicTimer);

                    charSpaceInjects = 0;
                }
                //10 + timingControl.Paddles.dah_ms + timingControl.Paddles.intraCharSpace_ms, injectCharSpace
            }
            else
            {
                ISR_Timer.changeInterval(iambicTimer, 10 + timingControl.Paddles.dah_ms + timingControl.Paddles.intraCharSpace_ms);
                ISR_Timer.restartTimer(iambicTimer);
                ISR_Timer.enable(iambicTimer);
                charSpaceInjects = 1;
            }
            */
        }
    }
}
#endif

#include <timerStuff/enableDisable.h>

#include <timerStuff/changeTimerWpm.h>

#include <timerStuff/timerInitialize.h>

#include <timerStuff/processDitDahQueue.h>

#endif