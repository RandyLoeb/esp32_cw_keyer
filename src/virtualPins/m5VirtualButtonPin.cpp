#include "m5VirtualButtonPin.h"
#include <M5Stack.h>

M5VirtualButtonPin::M5VirtualButtonPin(M5Btnslist btn, bool pressImpliesLow)
{
    this->btn = btn;
    this->_pressImpliesLow = pressImpliesLow;
};
int M5VirtualButtonPin::digitalRead()
{
    int isPressed = 0;
    //M5.update();
    switch (this->btn)
    {
    case M5Btnslist::A:
        isPressed = M5.BtnA.read();
        break;
    case M5Btnslist::B:
        isPressed = M5.BtnB.read();
        //isPressed = M5.BtnB.isPressed();
        break;
    case M5Btnslist::C:
        isPressed = M5.BtnC.read();
        break;
    }

    /* if (isPressed)
    {
        Serial.println("pressed!");
    } */
    if (_pressImpliesLow)
    {
        return !isPressed;
    }
    else
    {
        return isPressed;
    }
};
