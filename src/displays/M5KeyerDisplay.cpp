#include <string>
#include <M5Stack.h>
#include "M5KeyerDisplay.h"
#include "Free_Fonts.h"

void M5KeyerDisplay::displayUpdate(char character)
{
}
void M5KeyerDisplay::initialize()
{
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
            Serial.println("Ending timed display...");
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