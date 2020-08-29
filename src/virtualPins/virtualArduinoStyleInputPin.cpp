#include <Arduino.h>
#include "virtualArduinoStyleInputPin.h"
#include "virtualPin.h"

void VirtualArduinoStyleInputPin::initialize(int pinNumber, VitualArduinoPinPull pull)
{
    this->_pinNumber = pinNumber;
    this->_pull = pull;
    int mode = INPUT;
    //note 18 & 23 didn't seem to work as pullups on m5,
    //did not investigate why...
    if (this->_pull == VitualArduinoPinPull::VAPP_HIGH)
    {
        //Serial.println("Setting as pullup");
        mode = INPUT_PULLUP;
    }

    if (this->_pull == VitualArduinoPinPull::VAPP_LOW)
    {
        mode = INPUT_PULLDOWN;
    }
    pinMode(this->_pinNumber, mode);

    //testing
    /* int testRead = digitalRead(this->_pinNumber);
    Serial.print("testread on pin:");
    Serial.print(this->_pinNumber);
    Serial.print(" ");
    Serial.println(testRead);
    delay(5000); */
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
