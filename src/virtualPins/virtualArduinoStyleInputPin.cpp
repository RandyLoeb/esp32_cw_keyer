#include <Arduino.h>
#include "virtualArduinoStyleInputPin.h"
#include "virtualPin.h"

void VirtualArduinoStyleInputPin::initialize(int pinNumber, VitualArduinoPinPull pull)
{
    this->_pinNumber = pinNumber;
    this->_pull = pull;
    int mode = INPUT;
    if (this->_pull == VitualArduinoPinPull::VAPP_HIGH)
    {
        mode = INPUT_PULLUP;
    }

    if (this->_pull == VitualArduinoPinPull::VAPP_LOW)
    {
        mode = INPUT_PULLDOWN;
    }
    pinMode(this->_pinNumber, mode);
};

VirtualArduinoStyleInputPin::VirtualArduinoStyleInputPin(int pinNumber)
    : VirtualArduinoStyleInputPin(pinNumber, VitualArduinoPinPull::VAPP_NONE){};
VirtualArduinoStyleInputPin::VirtualArduinoStyleInputPin(int pinNumber, VitualArduinoPinPull pull)
{

    this->initialize(pinNumber, pull);
};
int VirtualArduinoStyleInputPin::digRead()
{
    return digitalRead(this->_pinNumber);
};
