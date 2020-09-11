#ifndef KEYERPINS_H
#define KEYERPINS_H
#include <Arduino.h>
#include "virtualPins.h"
VirtualPins virtualPins;
/* #include "virtualPins/m5VirtualButtonPin.h"
M5VirtualButtonPin btnA{M5Btnslist::A};
M5VirtualButtonPin btnC{M5Btnslist::C}; */
#include "virtualPins/virtualArduinoStyleInputPin.h"
VirtualArduinoStyleInputPin p2(2, VitualArduinoPinPull::VAPP_HIGH);
VirtualArduinoStyleInputPin p39(GPIO_NUM_39, VitualArduinoPinPull::VAPP_HIGH);

VirtualArduinoStyleInputPin p5(5, VitualArduinoPinPull::VAPP_HIGH);
VirtualArduinoStyleInputPin p37(GPIO_NUM_37, VitualArduinoPinPull::VAPP_HIGH);

/* VirtualPin &vpA{btnA};
VirtualPin &vpC{btnC}; */
VirtualPin &vp2{p2};
VirtualPin &vp5{p5};
VirtualPin &vp39{p39};
VirtualPin &vp37{p37};

#include "virtualPins/virtualPinSet.h"
VirtualPinSet ditPaddles;
VirtualPinSet dahPaddles;

#define VIRTUAL_DITS 0
#define VIRTUAL_DAHS 1

void initialize_virtualPins()
{

    ditPaddles.pins.push_back(&vp39);
    ditPaddles.pins.push_back(&vp2);
    dahPaddles.pins.push_back(&vp37);
    dahPaddles.pins.push_back(&vp5);

    virtualPins.pinsets.insert(std::make_pair(VIRTUAL_DITS, &ditPaddles));
    virtualPins.pinsets.insert(std::make_pair(VIRTUAL_DAHS, &dahPaddles));
}
#endif