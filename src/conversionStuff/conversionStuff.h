#ifndef CONVERSIONSTUFF_H
#define CONVERSIONSTUFF_H
#include <Arduino.h>
#include <vector>
#include <string>
#include "cwControl/cw_utils.h"
#include "displays/keyerDisplay.h"
#include "timerStuff/paddlePress.h"
#include "timerStuff/timingControl.h"
#include <ArduinoWebsockets.h>
#include "persistentConfig/persistentConfig.h"

//https://github.com/gilmaimon/ArduinoWebsockets?utm_source=platformio&utm_medium=piohome
using namespace websockets;
std::vector<PaddlePressDetection *> conversionQueue;
PaddlePressDetection *lastPress;
bool cachedWordSpace = false;
long lastWordLetterTimestamp = 0;
//persistentConfig *_timerStuffConfig;
bool convertDitsDahsToCharsAndSpaces(PaddlePressDetection *ditDahOrSpace, WebsocketsClient *client, persistentConfig *_timerStuffConfig, CwControl *_cwControl)
{

    bool charSpaceDetected = false;
    bool charInjected = false;
    bool isSpace = false;
    bool isForcedCharSpace = false;

    if (ditDahOrSpace->Display)
    {

        //bool wordSpaceDetected = false;
        bool isDummy = ditDahOrSpace->Detected == DitOrDah::DUMMY;

        isSpace = ditDahOrSpace->Detected == DitOrDah::SPACE;
        isForcedCharSpace = ditDahOrSpace->Detected == DitOrDah::FORCED_CHARSPACE;

        if (!(lastPress == nullptr))
        {

            /* if (lastPress->TimeStamp > ditDahOrSpace->TimeStamp)
            {
               
            } */
            long timeDiff = ditDahOrSpace->TimeStamp - lastPress->TimeStamp;

            if (isSpace)
            {

                //wordSpaceDetected = true;
                cachedWordSpace = true;
            }

            //was dit(60) + 60 + 30
            if (lastPress->Detected == DitOrDah::DIT && (timeDiff > (timingControl.Paddles.dit_ms + timingControl.Paddles.interCharSpace_ms)))
            {
                if (conversionQueue.size() > 0)
                {
                    //Serial.print("after dit time:");
                    //Serial.println(timeDiff);
                }

                //if (lastPress->Detected)
                charSpaceDetected = true;
            }

            if (lastPress->Detected == DitOrDah::DAH && (timeDiff > (timingControl.Paddles.dah_ms + timingControl.Paddles.interCharSpace_ms)))
            {
                if (conversionQueue.size() > 0)
                {
                    //Serial.print("after dah time:");
                    //Serial.println(timeDiff);
                }
                charSpaceDetected = true;
            }

            if (isForcedCharSpace)
            {
                charSpaceDetected = true;
            }

            /*
            if (isDummy)
            {
                Serial.println("dummycharspace");
                charSpaceDetected = true;
            }
            */
        }

        if (charSpaceDetected && conversionQueue.size() > 0)
        {

            long characterCode = 0;
            long lastTimeStamp = 0;
            //we got a space, time to analyze
            for (std::vector<PaddlePressDetection *>::iterator it = conversionQueue.begin(); it != conversionQueue.end(); ++it)
            {

                characterCode *= 10;
                if ((*it)->Detected == DitOrDah::DIT)
                {
                    characterCode += 1;
                }
                if ((*it)->Detected == DitOrDah::DAH)
                {
                    characterCode += 2;
                }

                String sdord = ((*it)->Detected == DitOrDah::DIT) ? "DIT" : "DAH";
                //Serial.print(sdord + ":");
                //Serial.print((*it)->TimeStamp);
                if (lastTimeStamp > 0)
                {
                    //Serial.print("(");
                    //Serial.print((*it)->TimeStamp - lastTimeStamp);
                    //Serial.print(")");
                }
                else if (lastWordLetterTimestamp > 0)
                {
                    //Serial.print("(");

                    long lastWordDiff = (*it)->TimeStamp - lastWordLetterTimestamp;
                    //Serial.print(lastWordDiff);
                    if (lastWordDiff > timingControl.Paddles.wordSpace_ms)
                    {
                        cachedWordSpace = true;
                    }
                    //Serial.print(")");
                }
                lastTimeStamp = (*it)->TimeStamp;
                //Serial.print(" ");
                delete (*it);
                //}
            }
            lastWordLetterTimestamp = lastTimeStamp;
            //Serial.println("");
            conversionQueue.clear();

            String cwCharacter = _cwControl->convert_cw_number_to_string(characterCode);
            if (cachedWordSpace)
            {
                displayControl.displayUpdate(" ");
                displayCache.add(" ");
                cachedWordSpace = false;
            }

#if !defined REMOTE_KEYER && defined REMOTE_CHARMODE
            //Serial.print("about to send sendTXT:");
            /*  Serial.println(webSocket.isConnected());
                webSocket.sendTXT(isDit ? "dit" : "dah"); */
            if (_timerStuffConfig->configJsonDoc["ws_connect"].as<int>() == 1 && _timerStuffConfig->configJsonDoc["ws_ip"].as<String>().length() > 0)
            {
                client->send(cwCharacter);
            }

#endif

            displayControl.displayUpdate(cwCharacter);
            displayCache.add(cwCharacter);
            charInjected = true;
        }

        if (!isDummy && !isSpace & !isForcedCharSpace)
        {
            conversionQueue.push_back(ditDahOrSpace);
            lastPress = ditDahOrSpace;
        }
        else
        {
            delete ditDahOrSpace;
        }
    }
    else
    {
        delete ditDahOrSpace;
    }

    return charInjected || isSpace || isForcedCharSpace;
}

#endif