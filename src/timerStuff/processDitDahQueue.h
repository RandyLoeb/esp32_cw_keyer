#ifndef PROCESSDITDAHQUEUE_H
#define PROCESSDITDAHQUEUE_H
#include <timerStuff/timerStuff.h>

bool needDummies = false;

void processDitDahQueue()
{

    bool convertDetectedCharSpace = true;
    bool lastWasDitOrDah = false;
    bool lastWasDummy = false;
    bool isDit = false;
    bool isDah = false;

#if !defined REMOTE_KEYER
    /* webSocket.loop(); */
    if (_timerStuffConfig->configJsonDoc["ws_connect"].as<int>() == 1 && _timerStuffConfig->configJsonDoc["ws_ip"].as<String>().length() > 0)
    {
        client.poll();
    }
    if (!clientIntiialized)
    {
        connectWsClient();
    }
#endif
#if !defined ESPNOW_ONLY && defined KEYER_WEBSERVER
    //deal with web server, as it may have turned off all timers
    //while it needed SPIFFS, and it needs "help"  to turn them
    //back on
    if (keyerWebServer->safeToTurnTimersBackOn > -1)
    {
        //i just used 1 second here and it worked, didn't try shorter,
        //maybe it can go lower....
        if ((millis() - keyerWebServer->safeToTurnTimersBackOn) > 1000)
        {
            //Serial.println("about to reenable timers");
            reEnableTimers();
            keyerWebServer->safeToTurnTimersBackOn = -1;
        }
    }
#endif

    while (!ditsNdahQueue.empty())
    {

        //soundPlaying is a kind of "lock" to make sure we wait for
        //the last sound, plus spacing to be done
        if (!soundPlaying)
        {
            PaddlePressDetection *paddePress = ditsNdahQueue.front();
            ditsNdahQueue.pop();

            isDit = paddePress->Detected == DitOrDah::DIT;
            isDah = paddePress->Detected == DitOrDah::DAH;
            lastWasDummy = paddePress->Detected == DitOrDah::DUMMY;

            if (isDit || isDah)
            {
                lastWasDitOrDah = true;
#if defined REMOTE_DITDAHMODE
#if defined M5CORE
                //sendEspNowDitDah(isDit ? ESPNOW_DIT : ESPNOW_DAH);
                /* String target = "http://192.168.0.129/";
                target += isDit ? "dit" : "dah";
                http.begin(target.c_str());
                http.GET() */
                //;

#if defined ESPNOW
                //Serial.println("Calling sendespnowditdah");
                sendEspNowDitDah(isDit ? ESPNOW_DIT : ESPNOW_DAH);
#endif

#endif

#if !defined REMOTE_KEYER
                //Serial.print("about to send sendTXT:");
                /*  Serial.println(webSocket.isConnected());
                webSocket.sendTXT(isDit ? "dit" : "dah"); */
                client.send(isDit ? "dit" : "dah");

#endif
#endif
            }
            // start playing tone
            // apparently m5 speaker just plays tone continuously, that's fine,
            // we have a timer to shut it off.

            if (paddePress->Detected != DitOrDah::DUMMY)
            {
                TimingSettings *timingSettings;

                if (paddePress->Source == PaddlePressSource::ARTIFICIAL)
                {
                    timingSettings = &timingControl.Farnsworth;
                    //Serial.println("using farnsworth");
                }
                else
                {
                    timingSettings = &timingControl.Paddles;
                    //Serial.println("using regular");
                }

                if (paddePress->Detected != DitOrDah::SPACE && paddePress->Detected != DitOrDah::FORCED_CHARSPACE)
                {
                    if (_timerStuffConfig->configJsonDoc["tx"].as<int>() > 0)
                    {
#if defined TRANSMIT
                        transmitControl.key();
#endif
                    }
#if defined TONE_ON
                    int hz = _timerStuffConfig->configJsonDoc["tone_hz"].as<int>();
                    if (hz > 0)
                    {
                        toneControl.tone(hz);
                    }
#endif
                }
                // lock us up
                soundPlaying = true;

                // figure out when the tone should stop
                // (and/or transmitter unkey)
                unsigned int interval = timingSettings->dit_ms;
                if (paddePress->Detected == DitOrDah::DAH)
                {
                    interval = timingSettings->dah_ms;
                }

                // if we are locking for intracharacter, i think we just account
                // for that
                if (paddePress->Detected == DitOrDah::SPACE)
                {
                    //interval = 360L;
                    interval = timingSettings->wordSpace_ms - timingSettings->intraCharSpace_ms;
                }
                if (paddePress->Detected == DitOrDah::FORCED_CHARSPACE)
                {
                    //interval = 180L;
                    interval = timingSettings->interCharSpace_ms - timingSettings->intraCharSpace_ms;
                }
                // set up and start the timer that will stop the tone
                // (and/or transmitter unkey)

                ISR_Timer.changeInterval(toneSilenceTimer, interval);
                ISR_Timer.restartTimer(toneSilenceTimer);
                ISR_Timer.enable(toneSilenceTimer);
            }
#if !defined REMOTE_KEYER
            convertDetectedCharSpace = convertDitsDahsToCharsAndSpaces(paddePress, &client, _timerStuffConfig, _cwControl);
#else
            convertDitsDahsToCharsAndSpaces(paddePress, nullptr, _timerStuffConfig, _cwControl);
#endif
        }
        else
        {
            //just wait until sound ends
        }
    }
    /*
    if (!convertDetectedCharSpace )
    {
        //TODO:This could be throttled, but I would rather do this
        //than use a timer
        //Serial.println("inserting dummy");
        //add dummies so characters can end
        PaddlePressDetection *newPd = new PaddlePressDetection();
        //newPd->Detected = DitOrDah::DUMMY;
        newPd->Detected = DitOrDah::DUMMY;
        ditsNdahQueue.push(newPd);
    }
    */
}

#endif