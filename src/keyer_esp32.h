#define GENERIC_CHARGRAB
//#define OLED_DISPLAY_64_128
#ifdef OLED_DISPLAY_64_128

// For a connection via I2C using the Arduino Wire include:
#include <Wire.h>        // Only needed for Arduino 1.6.5 and earlier
#include "SSD1306Wire.h" // legacy: #include "SSD1306.h"
int _DISPLAY_INITIALIZED = 0;
int _DISPLAY_HEIGHT = 0; //my little sisplay is 64x128
int _DISPLAY_WIDTH = 0;  //128

#define MYSDA 18
#define MYSDL 19
SSD1306Wire display(0x3c, MYSDA, MYSDL);
#endif



String displayContents = "";



void initDisplay()
{
#ifdef OLED_DISPLAY_64_128
    esp32_port_to_use->println("displayinitcalled");
    display.init();

    display.flipScreenVertically();
    //display.setFont(ArialMT_Plain_10);
    _DISPLAY_INITIALIZED = 1;
    display.clear();
    displayContents = "h=";
    _DISPLAY_HEIGHT = display.getHeight();
    displayContents += _DISPLAY_HEIGHT;
    displayContents += " ";
    displayContents += "w=";
    _DISPLAY_WIDTH = display.getWidth();
    displayContents += _DISPLAY_WIDTH;
    display.setTextAlignment(TEXT_ALIGN_LEFT);
    display.setFont(ArialMT_Plain_24);
    display.drawString(0, 0, displayContents);
    //display.drawString(0, 24, String(h) + " %");
    // write the buffer to the display
    display.display();
    displayContents = "";
#endif
}

void displayUpdate(char character)
{
    sendEspNowData(character);
#ifdef OLED_DISPLAY_64_128
    displayContents += character;
    //esp32_port_to_use->println("displayupdatecalled");
    if (!_DISPLAY_INITIALIZED)
    {
        initDisplay();
    }
    display.clear();

    display.setTextAlignment(TEXT_ALIGN_LEFT);
    display.setFont(ArialMT_Plain_24);
    int overflowing = 0;
    String displayLine1 = "";
    String displayLine2 = "";
    //String displayLine3 = "";

    do
    {
        if (overflowing)
        {
            int skip = displayLine1.length();
            /*String temp = "";
            for (int j = skip; j < displayContents.length(); j++)
            {
                temp += displayContents[j];
            }
            displayContents = temp;*/
            displayContents = displayContents.substring(skip);
            display.clear();
        }
        overflowing = 0;
        //int linesUsed = 0;
        displayLine1 = "";
        displayLine2 = "";
        //displayLine3 = "";

        for (int i = 0; i < displayContents.length(); i++)
        {
            //String temp = displayLine1;
            //temp +=
            //temp += displayContents[i];
            if (displayLine2.length() == 0 && (display.getStringWidth(displayLine1 + displayContents[i]) <= _DISPLAY_WIDTH))
            {
                displayLine1 += displayContents[i];
                //linesUsed = 1;
            }
            else if (display.getStringWidth(displayLine2 + displayContents[i]) <= _DISPLAY_WIDTH)
            {
                displayLine2 += displayContents[i];
                //linesUsed = 2;
            }
            /*else if (display.getStringWidth(displayLine3 + displayContents[i]) <= _DISPLAY_WIDTH)
            {
                displayLine3 += displayContents[i];
                linesUsed = 3;
            }*/
            else
            {
                overflowing = 1;
            }
        }
    } while (overflowing);

    display.drawString(0, 0, displayLine1);
    display.drawString(0, 29, displayLine2);
    //display.drawString(0, 58, displayLine3);
    //display.drawString(0, 24, String(h) + " %");
    // write the buffer to the display
    display.display();
#endif
}