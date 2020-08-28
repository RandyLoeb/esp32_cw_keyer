#include "m5VirtualButtonPin.h"
#include <M5Stack.h>

M5VirtualButtonPin::M5VirtualButtonPin(M5Btnslist btn)
{
    this->btn = btn;
};
int M5VirtualButtonPin::digitalRead()
{
    int isPressed =0;
    M5.update();
    switch (this->btn)
    {
    case M5Btnslist::A:
        isPressed= M5.BtnA.isPressed();
        break;
    case M5Btnslist::B:
        isPressed= M5.BtnB.isPressed();
        break;
    case M5Btnslist::C:
        isPressed= M5.BtnC.isPressed();
        break;
    }

    if (isPressed)
    {
        Serial.println("pressed!");
    }
    return isPressed;
};