#include <string>
#include <Wire.h>        // Only needed for Arduino 1.6.5 and earlier
#include "SSD1306Wire.h" // legacy: #include "SSD1306.h"
#include "displayBase.h"
#define MYSDA 18
#define MYSDL 19
class oled64x128 : public displayBase
{
    SSD1306Wire display;

public:
    oled64x128() : display(0x3c, MYSDA, MYSDL){};
};