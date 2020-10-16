#ifndef OLED64X128_H
#define OLED64X128_H
#include <string>
#include <Wire.h>        // Only needed for Arduino 1.6.5 and earlier
#include "SSD1306Wire.h" // legacy: #include "SSD1306.h"
#include "displayBase.h"
#define MYSDA 18
#define MYSDL 19
class oled64x128 : public displayBase
{
    String displayContents = "";
    int _DISPLAY_INITIALIZED = 0;
    int _DISPLAY_HEIGHT = 0; //my little sisplay is 64x128
    int _DISPLAY_WIDTH = 0;  //128
    SSD1306Wire display;
    String timedDisplayLine1 = "";
    String timedDisplayLine2 = "";
    int timedDelay = 0;
    int millisDelayStarted = 0;
    bool delayMode = false;

    void initDisplay();

public:
    oled64x128() : display(0x3c, MYSDA, MYSDL){};
    virtual void initialize();
    virtual void displayUpdate(char character);
    virtual void service_display();
    virtual void lcd_center_print_timed(String lcd_print_string, byte row_number, unsigned int duration);
    virtual int getColsCount();
    virtual int getRowsCount();
};

#endif