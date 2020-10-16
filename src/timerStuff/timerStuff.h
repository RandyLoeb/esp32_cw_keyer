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

#if !defined REMOTE_KEYER
/* #include <WebSocketsClient.h>
//https://github.com/Links2004/arduinoWebSockets
WebSocketsClient webSocket;

void webSocketClientEvent(WStype_t type, uint8_t *payload, size_t length)
{

    switch (type)
    {
    case WStype_DISCONNECTED:
        Serial.println("ws client disconnected.");
        //USE_SERIAL.printf("[WSc] Disconnected!\n");
        break;
    case WStype_CONNECTED:
        Serial.println("ws client connected.");
        //USE_SERIAL.printf("[WSc] Connected to url: %s\n", payload);

        // send message to server when Connected
        //webSocket.sendTXT("Connected");
        break;
    case WStype_TEXT:
        Serial.println("ws client text.");
        //USE_SERIAL.printf("[WSc] get text: %s\n", payload);

        // send message to server
        // webSocket.sendTXT("message here");
        break;
    case WStype_BIN:
        Serial.println("ws client bin.");
        //USE_SERIAL.printf("[WSc] get binary length: %u\n", length);
        //hexdump(payload, length);

        // send data to server
        // webSocket.sendBIN(payload, length);
        break;
    case WStype_ERROR:
        Serial.println("ws client error.");
        break;
    case WStype_FRAGMENT_TEXT_START:
    case WStype_FRAGMENT_BIN_START:
    case WStype_FRAGMENT:
    case WStype_FRAGMENT_FIN:
        break;
    }
} */
#include <ArduinoWebsockets.h>
//https://github.com/gilmaimon/ArduinoWebsockets?utm_source=platformio&utm_medium=piohome
using namespace websockets;
WebsocketsClient client;
bool clientIntiialized = false;

void connectWsClient()
{
    if (_timerStuffConfig->configJsonDoc["ws_connect"].as<int>() == 1 && _timerStuffConfig->configJsonDoc["ws_ip"].as<String>().length() > 0)
    {
        // Connect to server
        client.connect(_timerStuffConfig->configJsonDoc["ws_ip"].as<String>(), 80, "/ws");
    }
}

void onMessageCallback(WebsocketsMessage message)
{
    Serial.print("Got Message: ");
    Serial.println(message.data());
}

void onEventsCallback(WebsocketsEvent event, String data)
{
    if (event == WebsocketsEvent::ConnectionOpened)
    {
        Serial.println("Connnection Opened");
        clientIntiialized = true;
    }
    else if (event == WebsocketsEvent::ConnectionClosed)
    {
        Serial.println("Connnection Closed");
        clientIntiialized = false;
    }
    else if (event == WebsocketsEvent::GotPing)
    {
        Serial.println("Got a Ping!");
    }
    else if (event == WebsocketsEvent::GotPong)
    {
        Serial.println("Got a Pong!");
    }
}

#endif

#if defined REMOTE_UDP
#if defined M5CORE
#include "AsyncUDP.h"
AsyncUDP udp;
#endif
#endif
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

// timers kick off these two funcs below.
// has debugging to see if we are dead nuts accurate
void IRAM_ATTR doDits()
{

    PaddlePressDetection *newPd = new PaddlePressDetection();
    newPd->Detected = DitOrDah::DIT;
    ditsNdahQueue.push(newPd);
#if (TIMER_INTERRUPT_DEBUG > 0)

    Serial.print("dit = ");
    Serial.println(deltaMillis);
#endif

    //previousMillis = millis();
}

void IRAM_ATTR doDahs()
{

    PaddlePressDetection *newPd = new PaddlePressDetection();
    newPd->Detected = DitOrDah::DAH;
    ditsNdahQueue.push(newPd);
#if (TIMER_INTERRUPT_DEBUG > 0)
    Serial.print("dah = ");
    Serial.println(deltaMillis);
#endif

    //previousMillis = millis();
}

// holds our flags and timer handles
volatile bool detectInterrupts = false;
volatile bool ditPressed = false;
volatile bool dahPressed = false;
volatile int ditTimer;
volatile int dahTimer;
volatile int debounceDitTimer;
volatile int debounceDahTimer;
volatile int toneSilenceTimer;
volatile int ditDahSpaceLockTimer;
volatile int charSpaceTimer;
volatile bool ditLocked = false;
volatile bool dahLocked = false;

// this is triggerd by hardware interrupt indirectly
void IRAM_ATTR detectPress(volatile bool *locker, volatile bool *pressed, int timer, int oppoTimer, int lockTimer, int pin, DitOrDah message)
{

    // locker is our debouce variable, i.e. we'll ignore any changes
    // to the pin during hte bounce
    if (!*locker)
    {

        // wait for interrupts to be turned on
        if (detectInterrupts)
        {

            // what was previous date of pin?
            int pressedBefore = *pressed;

            // get the pin
            *pressed = !virtualPins.pinsets[pin]->digRead();
            // pressed?
            if (*pressed && !pressedBefore)
            {

                //for now iambic behavior is a press stops press of the other
                ISR_Timer.disable(oppoTimer);

                PaddlePressDetection *newPd = new PaddlePressDetection();
                newPd->Detected = message;
                ditsNdahQueue.push(newPd);
#if (TIMER_INTERRUPT_DEBUG > 0)

                //Serial.println(message + "pressed");
#endif

                //kickoff either the dit or dah timer passed in
                //so it will keep injecting into the queue
                ISR_Timer.restartTimer(timer);
                ISR_Timer.enable(timer);
            }
            // released?
            else if (!*pressed && pressedBefore)
            {
                // released so stop the timer
                ISR_Timer.disable(timer);
            }
        }

        // lock and kickoff the debouncer
        *locker = true;
        ISR_Timer.restartTimer(lockTimer);
        ISR_Timer.enable(lockTimer);
    }
}

// these two are triggered by hardware interrupts
void IRAM_ATTR detectDitPress()
{
    detectPress(&ditLocked, &ditPressed, ditTimer, dahTimer, debounceDitTimer, VIRTUAL_DITS, DitOrDah::DIT);
}

void IRAM_ATTR detectDahPress()
{
    detectPress(&dahLocked, &dahPressed, dahTimer, ditTimer, debounceDahTimer, VIRTUAL_DAHS, DitOrDah::DAH);
}

// fired by the timer that unlocks the debouncer indirectly
void IRAM_ATTR unlockDebouncer(void (*detectCallback)(), volatile bool *flagToFalse)
{
    *flagToFalse = false;
    detectCallback();
}

// fired by timer for unlocker direclty
void IRAM_ATTR unlockDit()
{
    unlockDebouncer(detectDitPress, &ditLocked);
}

void IRAM_ATTR unlockDah()
{
    unlockDebouncer(detectDahPress, &dahLocked);
}

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

// fired by timer holds sound during dits & dah spacing
void IRAM_ATTR releaseLockForDitDahSpace()
{

    soundPlaying = false;
    ISR_Timer.disable(ditDahSpaceLockTimer);
}

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

void disableAllTimers()
{

    ISR_Timer.disableAll();
    Serial.println("disabled all timers");
}

void reEnableTimers()
{
    Serial.println("reenabling timers");
    ISR_Timer.enable(charSpaceTimer);

//not sure these need to be re-enabled, but maybe causing problem with slave that doesn't have pin interrupts?
#if !defined REMOTE_KEYER
    Serial.println("re-enabling debouncers");
    ISR_Timer.enable(debounceDitTimer);
    ISR_Timer.enable(debounceDahTimer);
#endif
}

void changeTimerWpm()
{

    timingControl.setWpm(configControl.configJsonDoc["wpm"].as<int>(), configControl.configJsonDoc["wpm_farnsworth"].as<int>(), configControl.configJsonDoc["wpm_farnsworth_slow"].as<int>());
    ISR_Timer.disable(ditTimer);
    ISR_Timer.disable(dahTimer);
    ISR_Timer.disable(toneSilenceTimer);
    ISR_Timer.disable(ditDahSpaceLockTimer);
    ISR_Timer.disable(charSpaceTimer);

    ISR_Timer.changeInterval(ditTimer, 1 + timingControl.Paddles.dit_ms + timingControl.Paddles.intraCharSpace_ms);
    ISR_Timer.changeInterval(dahTimer, 1 + timingControl.Paddles.dah_ms + timingControl.Paddles.intraCharSpace_ms);
    ISR_Timer.changeInterval(toneSilenceTimer, timingControl.Paddles.dit_ms);
    ISR_Timer.changeInterval(ditDahSpaceLockTimer, timingControl.Paddles.intraCharSpace_ms);
    ISR_Timer.changeInterval(charSpaceTimer, 10 + timingControl.Paddles.dah_ms + timingControl.Paddles.intraCharSpace_ms);

    ISR_Timer.disable(ditTimer);
    ISR_Timer.disable(dahTimer);
    ISR_Timer.disable(toneSilenceTimer);
    ISR_Timer.disable(ditDahSpaceLockTimer);
    ISR_Timer.enable(charSpaceTimer);
}

void initializeTimerStuff(persistentConfig *_config, CwControl *cwControl)
{

    _timerStuffConfig = _config;
    _cwControl = cwControl;
#if defined TRANSMIT
    Serial.println("initialzing transmit control");
    transmitControl.initialize(_timerStuffConfig);
    Serial.println("transmit control initialized");
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
        Serial.println("Starting  ITimer OK, millis() = " + String(lastMillis));
    }
    else
        Serial.println("Can't set ITimer correctly. Select another freq. or interval");

    // Just to demonstrate, don't use too many ISR Timers if not absolutely necessary

    // this timer monitors the dit paddle held down
    ditTimer = ISR_Timer.setInterval(1 + timingControl.Paddles.dit_ms + timingControl.Paddles.intraCharSpace_ms, doDits);

    // this timer monitors the dah paddle held down
    dahTimer = ISR_Timer.setInterval(1 + timingControl.Paddles.dah_ms + timingControl.Paddles.intraCharSpace_ms, doDahs);

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

    // not sure if disabled by default by do it
    ISR_Timer.disable(ditTimer);
    ISR_Timer.disable(dahTimer);
    ISR_Timer.disable(debounceDitTimer);
    ISR_Timer.disable(debounceDahTimer);
    ISR_Timer.disable(toneSilenceTimer);
    ISR_Timer.disable(ditDahSpaceLockTimer);
    ISR_Timer.enable(charSpaceTimer);
    Serial.println("Timer stuff initialized.");
}

void processDitDahQueue()
{

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
            Serial.println("about to reenable timers");
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

            bool isDit = paddePress->Detected == DitOrDah::DIT;
            bool isDah = paddePress->Detected == DitOrDah::DAH;

            if (isDit || isDah)
            {
#if defined REMOTE_DITDAHMODE
#if defined M5CORE
                //sendEspNowDitDah(isDit ? ESPNOW_DIT : ESPNOW_DAH);
                /* String target = "http://192.168.0.129/";
                target += isDit ? "dit" : "dah";
                http.begin(target.c_str());
                http.GET() */
                //;

#if defined REMOTE_UDP
                udp.broadcastTo(isDit ? "dit" : "dah", 1234);
#endif

#if defined ESPNOW
                Serial.println("Calling sendespnowditdah");
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
            convertDitsDahsToCharsAndSpaces(paddePress, &client, _timerStuffConfig, _cwControl);
#else
            convertDitsDahsToCharsAndSpaces(paddePress, nullptr, _timerStuffConfig, _cwControl);
#endif
        }
        else
        {
            //just wait until sound ends
        }
    }
}
#endif