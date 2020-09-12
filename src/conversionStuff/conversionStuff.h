#ifndef CONVERSIONSTUFF_H
#define CONVERSIONSTUFF_H
#include <Arduino.h>
#include <vector>
#include <string>
#include "cw_utils.h"
#include "displays/keyerDisplay.h"
#include "timerStuff/paddlePress.h"
std::vector<PaddlePressDetection *> conversionQueue;
PaddlePressDetection *lastPress;
bool cachedWordSpace = false;
long lastWordLetterTimestamp = 0;
void convertDitsDahsToCharsAndSpaces(PaddlePressDetection *ditDahOrSpace)
{

    if (ditDahOrSpace->Display)
    {
        bool charSpaceDetected = false;
        //bool wordSpaceDetected = false;
        bool isDummy = ditDahOrSpace->Detected == DitOrDah::DUMMY;
        //if (isDummy) {Serial.println("is dummy");}
        bool isSpace = ditDahOrSpace->Detected == DitOrDah::SPACE;
        bool isForcedCharSpace = ditDahOrSpace->Detected == DitOrDah::FORCED_CHARSPACE;

        if (!(lastPress == nullptr))
        {

            /* if (lastPress->TimeStamp > ditDahOrSpace->TimeStamp)
            {
                Serial.println("ROLLOVER! ROLLOVER!");
            } */
            long timeDiff = ditDahOrSpace->TimeStamp - lastPress->TimeStamp;

            if (isSpace)
            {
                //Serial.print("Caching a wordspace:");
                //Serial.println(timeDiff);
                //wordSpaceDetected = true;
                cachedWordSpace = true;
            }

            if (lastPress->Detected == DitOrDah::DIT && (timeDiff > (60 + 60 + 30)))
            {
                if (conversionQueue.size() > 0)
                {
                    Serial.print("after dit time:");
                    Serial.println(timeDiff);
                }
                charSpaceDetected = true;
            }

            if (lastPress->Detected == DitOrDah::DAH && (timeDiff > (180 + 60 + 30)))
            {
                if (conversionQueue.size() > 0)
                {
                    Serial.print("after dah time:");
                    Serial.println(timeDiff);
                }
                charSpaceDetected = true;
            }

            if (isForcedCharSpace)
            {
                charSpaceDetected = true;
            }
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
                Serial.print(sdord + ":");
                Serial.print((*it)->TimeStamp);
                if (lastTimeStamp > 0)
                {
                    Serial.print("(");
                    Serial.print((*it)->TimeStamp - lastTimeStamp);
                    Serial.print(")");
                }
                else if (lastWordLetterTimestamp > 0)
                {
                    Serial.print("(");

                    long lastWordDiff = (*it)->TimeStamp - lastWordLetterTimestamp;
                    Serial.print(lastWordDiff);
                    if (lastWordDiff > 240)
                    {
                        cachedWordSpace = true;
                    }
                    Serial.print(")");
                }
                lastTimeStamp = (*it)->TimeStamp;
                Serial.print(" ");
                delete (*it);
                //}
            }
            lastWordLetterTimestamp = lastTimeStamp;
            Serial.println("");
            conversionQueue.clear();

            String cwCharacter = convert_cw_number_to_string(characterCode);
            if (cachedWordSpace)
            {
                displayControl.displayUpdate(" ");
                cachedWordSpace = false;
            }
            displayControl.displayUpdate(cwCharacter);
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
}

#endif