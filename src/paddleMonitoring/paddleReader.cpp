#include <Arduino.h>
#include "paddleReading.h"
#include "paddleReader.h"

bool PaddleReader::analyzeTimePassage(int ditsPassed, bool ditPressed, bool dahPressed)
{
    bool makeCurrentLast = false;

    if (ditPressed)
    {
        if (ditsPassed >= 2)
        {
            //we've have a dit plus a space so inject another dit
            this->inject(PaddleReader::injections::dit);
            makeCurrentLast = true;
        }
    }
    else if (dahPressed)
    {
        if (ditsPassed >= 4)
        {
            //we've reached a dah plus a space so inject another dah
            this->inject(PaddleReader::injections::dah);
            makeCurrentLast = true;
        }
    }
    else
    {
        //silence
        if (ditsPassed >= 2)
        {

            if (this->lastInjection != PaddleReader::injections::char_space && this->lastInjection != PaddleReader::injections::word_space)
            {
                // last was not a space so at most this is a char space
                this->inject(PaddleReader::injections::char_space);
                makeCurrentLast = true;
            }
            else if (this->lastInjection == PaddleReader::injections::char_space)
            {
                //we were on a char space, the window has passed so
                // it's a word_space
                this->inject(PaddleReader::injections::word_space);
                makeCurrentLast = true;
            }
            else
            {
                // we're on a word_space so just sit tight and do nothing...
            }
        }
    }

    return makeCurrentLast;
}

void PaddleReader::inject(injections injection)
{
    this->lastInjection = injection;
    switch (injection)
    {
    case dit:
        //Serial.print(".");
        this->bufferedDitsDahs = this->bufferedDitsDahs + ".";
        break;
    case dah:
        //Serial.print("-");
        this->bufferedDitsDahs = this->bufferedDitsDahs + "-";
        break;
    case char_space:
        this->bufferedDitsDahs = this->bufferedDitsDahs + " ";
        break;
    case word_space:
        Serial.println(this->bufferedDitsDahs + "<endword>");
        this->bufferedDitsDahs = "";
        break;
    default:
        break;
    }
}

int (*_readDitPaddle)();
int (*_readDahPaddle)();

PaddleReader::PaddleReader(int (*readDitPaddle)(), int (*readDahPaddle)(), int wpm)
{
    Serial.println("About to initialize paddle reader..");
    this->_readDitPaddle = readDitPaddle;
    this->_readDahPaddle = readDahPaddle;

    Serial.println("About to set wpm...");
    this->setWpm(wpm);
    Serial.println("wpm was set initially");
    this->bufferedDitsDahs = "";
}
void PaddleReader::setWpm(int wpm)
{
    this->_wpm = wpm;
    Serial.print("wpm:");
    Serial.println(wpm);

    double minutesPerWord = 1 / (double)wpm;
    Serial.print("minutesPerWord:");
    Serial.println(minutesPerWord);

    double secondsPerWord = 60 * minutesPerWord;
    Serial.print("secondsPerWord:");
    Serial.println(secondsPerWord);
    Serial.print("ditsperword:");
    Serial.println(_ditsPerWord);
    double secondsPerDit = secondsPerWord / this->_ditsPerWord;
    Serial.print("secondsPerdit:");
    Serial.println(secondsPerDit);
    this->_millisecondsPerDit = secondsPerDit * 1000;
    Serial.print("mspd:");
    Serial.println(this->_millisecondsPerDit);
}

void PaddleReader::readPaddles()
{
    //PaddleReading *activeReading = new PaddleReading();
    long const millisNow = millis();
    int ditPressed = this->interruptDitPressed; //!this->_readDitPaddle();
    int dahPressed = this->interruptDahPressed; //!this->_readDahPaddle();
    bool makeCurrentLast = false;

    if (lastReadingState == NULL)
    {
        //this is the new state
        Serial.println("passed the initial null");
        lastReadingState = new PaddleReading();
        makeCurrentLast = true;
    }
    else
    {
        /* Serial.print("millisNow:");
        Serial.println(millisNow);
        Serial.print("lastmillis:");
        Serial.println(this->lastReadingState->startMillis); */

        long millisSinceLast = millisNow - this->lastReadingState->startMillis;
        /* Serial.print("millisSinceLast:");
        Serial.println(millisSinceLast); */

        int ditsPassed = millisSinceLast / (double)this->_millisecondsPerDit;
        /* Serial.print("ditspassed:");
        Serial.println(ditsPassed);
        delay(1000); */

        bool lastSilent = !this->lastReadingState->dahPaddlePressed && !this->lastReadingState->ditPaddlePressed;
        bool nowSilent = !(ditPressed || dahPressed);
        bool nowBoth = ditPressed && dahPressed;
        /* if (nowBoth)
        {
            Serial.println("nowboth");
        } */
        bool lastDit = this->lastReadingState->ditPaddlePressed;
        bool lastDah = this->lastReadingState->dahPaddlePressed;

        if (nowBoth)
        {
            /* ditPressed = false;
            dahPressed = false; */

             if (this->ditLockedOut || this->dahLockedOut)
            {
                if (this->ditLockedOut)
                {
                    ditPressed = false;
                }
                else if (this->dahLockedOut)
                {
                    dahPressed = false;
                }
            }
            else
            {

                if (lastSilent)
                {
                    dahPressed = false;
                }
                else if (lastDit)
                {
                    this->ditLockedOut = true;
                    ditPressed = false;
                }
                else if (lastDah)
                {
                    this->dahLockedOut = true;
                    dahPressed = false;
                }
            }
        }
        else
        {
            this->ditLockedOut = false;
            this->dahLockedOut = false;
        }

        bool changedSinceLast =
            (lastSilent && !nowSilent) ||
            (lastDit && !ditPressed) ||
            (lastDah && !dahPressed);
        //compare
        if (changedSinceLast)
        {
            //Serial.print("cd");
            //this->analyzeTimePassage(ditsPassed, this->lastReadingState->ditPaddlePressed, this->lastReadingState->dahPaddlePressed);
            //something changed
            makeCurrentLast = true;
            //analyze
            if (ditPressed)
            {

                this->inject(PaddleReader::injections::dit);
            }
            else if (dahPressed)
            {
                this->inject(PaddleReader::injections::dah);
            }
            else
            {
                //silence, handle it later loops...
            }
        }
        else
        {
            //nothing changed
            makeCurrentLast = this->analyzeTimePassage(ditsPassed, ditPressed, dahPressed);
        }
    }

    if (makeCurrentLast)
    {

        lastReadingState->ditPaddlePressed = ditPressed;
        lastReadingState->dahPaddlePressed = dahPressed;
        lastReadingState->startMillis = millis();
    }
}
