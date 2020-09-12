#include <string>
#include <M5Stack.h>
#include "M5KeyerDisplay.h"
#include "Free_Fonts.h"

void M5KeyerDisplay::displayUpdate(String character, bool safeScroll)
{
    String testLine;
    String testChar{character};
    bool isSpace = false;
    bool isNewLine = false;

    if (testChar == " ")
    {
        isSpace = true;
    }

    if (testChar == "\n")
    {
        isNewLine = true;
    }

    M5.Lcd.setTextDatum(TL_DATUM);
    M5.Lcd.setTextColor(TFT_WHITE, TFT_BLACK);
    M5.Lcd.setFreeFont(FF20);

    //note this seems accurate whereas full strings not?
    int testCharWidth = isSpace ? M5.Lcd.textWidth("Q") : M5.Lcd.textWidth(testChar);
    if (!safeScroll)
    {
        bool willOverFlow = (_cursorX + testCharWidth) > M5.Lcd.width();
        if (willOverFlow)
        {
            if (isSpace && !this->_cachedSpace)
            {
                //cache it and deal with it next time
                this->_cachedSpace = true;
            }
            else
            {
                this->_cachedSpace = false;
                if (!safeScroll)
                {
                    String cpy = String(this->_currentLine);
                    // put the current line into the queue

                    /* Serial.println("here's what I'm putting into the screen queue:");
                    Serial.print("'");
                    Serial.print(cpy);
                    Serial.println("'"); */
                    this->_lines.push_back(cpy);
                }

                // clear the current line
                this->_currentLine = "";
                if (safeScroll || (this->_Row < (this->_maxRows - 1)))
                {
                    //we're not at the bottom yet so just push it in
                    this->_Row++;
                }
                else
                {
                    //we're full so loose the oldest

                    this->_lines.erase(this->_lines.begin());

                    M5.Lcd.fillScreen(BLACK);
                    this->_Row = 0;
                    this->_cursorX = 0;

                    //now go through each line, char by char and refill the screen
                    //Serial.println("here's the screen queue:");
                    for (String line : this->_lines)
                    {
                        int length = line.length();
                        for (int j = 0; j < length; j++)
                        {
                            String currentScreenLetter = line.substring(j, j + 1);
                            //Serial.print(currentScreenLetter);
                            this->displayUpdate(currentScreenLetter, true);
                        }
                        //Serial.println("");
                        this->_cursorX = 0;
                        this->_Row++;
                    }
                    this->_Row = this->_maxRows - 1;
                }
                this->_cursorX = 0;
                this->_currentLine = "";
            }
        }

        if (!this->_cachedSpace)
        {
            if (!isSpace)
            {
                M5.Lcd.drawString(testChar, this->_cursorX, this->_Row * 50, GFXFF);
            }
            this->_cursorX += testCharWidth;
            this->_currentLine += testChar;
        }
    }
    else
    {

        //just print it as is
        if (!isSpace && !isNewLine)
        {
            M5.Lcd.drawString(testChar, this->_cursorX, this->_Row * 50, GFXFF);
        }

        if (!isNewLine)
        {
            this->_cursorX += testCharWidth;
        }

        //this->_currentLine += testChar;
    }
}

void M5KeyerDisplay::displayUpdate(char character)
{
    if (this->_cachedSpace)
    {
        this->displayUpdate(" ", false);
    }
    this->displayUpdate(String(character), false);
}

void M5KeyerDisplay::service_display()
{
    if (delayMode)
    {
        /* if (!_DISPLAY_INITIALIZED)
        {
            initDisplay();
        } */

        // text print

        //M5.Lcd.setCursor(10, 10);
        //M5.Lcd.setTextColor(WHITE);
        //M5.Lcd.setTextSize(5);
        if (lastTimedDisplayLine1 != timedDisplayLine1)
        {
            M5.Lcd.drawString(timedDisplayLine1, 160, 60, GFXFF);
            lastTimedDisplayLine1 = timedDisplayLine1;
        }

        if (lastTimedDisplayLine2 != timedDisplayLine2)
        {
            M5.Lcd.drawString(timedDisplayLine2, 160, 120, GFXFF);
            lastTimedDisplayLine2 = timedDisplayLine2;
        }

        //M5.Lcd.drawString(timedDisplayLine1, 10, 10);
        // M5.Lcd.setCursor(10, 40);
        //M5.Lcd.drawString(timedDisplayLine2, 10, 40);
        /* display.clear();

        display.setTextAlignment(TEXT_ALIGN_LEFT);
        display.setFont(ArialMT_Plain_24);
        display.drawString(0, 0, timedDisplayLine1);
        display.drawString(0, 29, timedDisplayLine2);
        display.display(); */

        if ((millis() - millisDelayStarted) > timedDelay)
        {
            //Serial.println("Ending timed display...");
            delayMode = false;
            M5.Lcd.fillScreen(BLACK);
            lastTimedDisplayLine1 = "";
            lastTimedDisplayLine2 = "";
            //display.clear();

            //display.display();
        }
    }
};
void M5KeyerDisplay::lcd_center_print_timed(String lcd_print_string, byte row_number, unsigned int duration)
{
    /* Serial.print("oled center print:'");
    Serial.print(lcd_print_string);
    Serial.print("' line:");
    Serial.print(row_number);
    Serial.print(" duration:");
    Serial.println(duration); */

    if (row_number == 0)
    {
        timedDisplayLine1 = lcd_print_string;
    }
    else
    {
        timedDisplayLine2 = lcd_print_string;
    }
    timedDelay = duration;
    millisDelayStarted = millis();
    if (!delayMode)
    {
        // Set text datum to middle centre
        M5.Lcd.setTextDatum(MC_DATUM);

        // Set text colour to orange with black background
        M5.Lcd.setTextColor(TFT_WHITE, TFT_BLACK);
        M5.Lcd.fillScreen(BLACK);
        //#define FSS24 &FreeSans24pt7b
        //#define FF20 &FreeSans24pt7b
        //M5.Lcd.setFreeFont(FF18);                 // Select the font
        //M5.Lcd.drawString(sFF20, 160, 60, GFXFF); // Print the string name of the font
        M5.Lcd.setFreeFont(FF20);
        //M5.Lcd.drawString(TEXT, 160, 120, GFXFF);
    }
    delayMode = true;
};
int M5KeyerDisplay::getColsCount() { return 0; };
int M5KeyerDisplay::getRowsCount() { return 2; };