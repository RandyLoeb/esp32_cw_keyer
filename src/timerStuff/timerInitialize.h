
#ifndef TIMERINITIALIZE_H
#define TIMERINITIALIZE_H
#include <timerStuff/timerStuff.h>

void initializeTimerStuff(persistentConfig *_config, CwControl *cwControl)
{

    _timerStuffConfig = _config;
    _cwControl = cwControl;
#if defined TRANSMIT
    //Serial.println("initialzing transmit control");
    transmitControl.initialize(_timerStuffConfig);
    //Serial.println("transmit control initialized");
#endif

#if !defined REMOTE_KEYER
    /* // server address, port and URL
    webSocket.beginSocketIO("ws://192.168.0.129", 80, "/ws");
    

    // event handler
    webSocket.onEvent(webSocketClientEvent);

    // use HTTP Basic Authorization this is optional remove if not needed
    //webSocket.setAuthorization("user", "Password");

    // try ever 5000 again if connection has failed
    webSocket.setReconnectInterval(5000); */
    // run callback when messages are received
    client.onMessage(onMessageCallback);

    // run callback when events are occuring
    client.onEvent(onEventsCallback);

    // Connect to server
    connectWsClient();

#endif

    if (configControl.configJsonDoc["wpm_farnsworth"].as<int>() <= 0)
    {
        configControl.configJsonDoc["wpm_farnsworth"] = configControl.configJsonDoc["wpm"].as<int>();
    }

    if (configControl.configJsonDoc["wpm_farnsworth_slow"].as<int>() <= 0)
    {
        configControl.configJsonDoc["wpm_farnsworth_slow"] = configControl.configJsonDoc["wpm"].as<int>();
    }

    timingControl.setWpm(configControl.configJsonDoc["wpm"].as<int>(), configControl.configJsonDoc["wpm_farnsworth"].as<int>(), configControl.configJsonDoc["wpm_farnsworth_slow"].as<int>());

    // have to play nice with the configControl and turn off
    // timers when saving to spiffs otherwise we get kernal panics
    configControl.wpmChangeCallbacks.push_back(changeTimerWpm);
    configControl.preSaveCallbacks.push_back(disableAllTimers);
    configControl.postSaveCallbacks.push_back(reEnableTimers);

#if !defined ESPNOW_ONLY && defined KEYER_WEBSERVER
    keyerWebServer->preSPIFFSCallbacks.push_back(disableAllTimers);
    keyerWebServer->postSPIFFSCallbacks.push_back(reEnableTimers);
#endif

#if defined HAS_DITDAHPINS
#if defined M5CORE && defined DITPIN1 && defined DAHPIN1
    pinMode(DITPIN1, INPUT_PULLUP);
    pinMode(DAHPIN1, INPUT_PULLUP);
#endif

//seems like pin can only have one interrupt attached
#if defined DITPIN1
    attachInterrupt(digitalPinToInterrupt(DITPIN1), detectDitPress, CHANGE);
#endif
#if defined DITPIN2
    attachInterrupt(digitalPinToInterrupt(DITPIN2), detectDitPress, CHANGE);
#endif

#if defined DAHPIN1
    attachInterrupt(digitalPinToInterrupt(DAHPIN1), detectDahPress, CHANGE);
#endif

#if defined DAHPIN2
    attachInterrupt(digitalPinToInterrupt(DAHPIN2), detectDahPress, CHANGE);
#endif
#endif

    // Using ESP32  => 80 / 160 / 240MHz CPU clock ,
    // For 64-bit timer counter
    // For 16-bit timer prescaler up to 1024

    // Interval in microsecs
    if (ITimer.attachInterruptInterval(HW_TIMER_INTERVAL_MS * 1000, TimerHandler))
    {
        lastMillis = millis();
        //Serial.println("Starting  ITimer OK, millis() = " + String(lastMillis));
    }
    else
    {
        //Serial.println("Can't set ITimer correctly. Select another freq. or interval");
    }
    // Just to demonstrate, don't use too many ISR Timers if not absolutely necessary

    currentDitTiming = 1 + timingControl.Paddles.dit_ms + timingControl.Paddles.intraCharSpace_ms;
    currentDahTiming = 1 + timingControl.Paddles.dah_ms + timingControl.Paddles.intraCharSpace_ms;

    // debouncers, needs some tweaking
    debounceDitTimer = ISR_Timer.setInterval(5L, unlockDit);
    debounceDahTimer = ISR_Timer.setInterval(10L, unlockDah);

    // timer to silence tone (could be used to unkey transmitter)
    // note this will be changed depending on dit dah but just
    // give initial value here
    toneSilenceTimer = ISR_Timer.setInterval(timingControl.Paddles.dit_ms, silenceTone);

    // timer to unlock the sidetone (could be transmitter key)
    // another way to think of this is as intra-character timer?
    ditDahSpaceLockTimer = ISR_Timer.setInterval(timingControl.Paddles.intraCharSpace_ms, releaseLockForDitDahSpace);

    charSpaceTimer = ISR_Timer.setInterval(10 + timingControl.Paddles.dah_ms + timingControl.Paddles.intraCharSpace_ms, injectCharSpace);

#if defined IAMBIC_ALTERNATE
    baseIambicTiming = timingControl.Paddles.intraCharSpace_ms;
    iambicTimer = ISR_Timer.setInterval(baseIambicTiming, iambicAction);
#endif

    // not sure if disabled by default by do it
    //ISR_Timer.disable(ditTimer);
    //ISR_Timer.disable(dahTimer);
    ISR_Timer.disable(debounceDitTimer);
    ISR_Timer.disable(debounceDahTimer);
    ISR_Timer.disable(toneSilenceTimer);
    ISR_Timer.disable(ditDahSpaceLockTimer);
    ISR_Timer.enable(charSpaceTimer);
#if defined IAMBIC_ALTERNATE
    ISR_Timer.disable(iambicTimer);
#endif

    //Serial.println("Timer stuff initialized.");
}

#endif