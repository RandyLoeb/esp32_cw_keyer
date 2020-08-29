#ifndef VIRTUALARDUINOSTYLEPIN_H
#define VIRTUALARDUINOSTYLEPIN_H
#include "virtualPin.h"
enum VitualArduinoPinPull
{
    VAPP_NONE = 0,
    VAPP_LOW = 1,
    VAPP_HIGH = 2
};
class VirtualArduinoStyleInputPin : public VirtualPin
{
    int _pinNumber;
    VitualArduinoPinPull _pull;
    void initialize(int pinNumber, VitualArduinoPinPull pull);

public:
    VirtualArduinoStyleInputPin(int pinNumber);
    VirtualArduinoStyleInputPin(int pinNumber, VitualArduinoPinPull pull);
    virtual int digRead();
};
#endif