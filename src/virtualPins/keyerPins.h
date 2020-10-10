#ifndef KEYERPINS_H
#define KEYERPINS_H
#include <Arduino.h>
#include "keyer_esp32.h"
#include "virtualPins.h"
#include "virtualPins/virtualArduinoStyleInputPin.h"
#include "virtualPins/virtualPinSet.h"
VirtualPins virtualPins;

#if defined DITPIN1
VirtualArduinoStyleInputPin ditp1(DITPIN1, VitualArduinoPinPull::VAPP_HIGH);
VirtualPin &vditp1{ditp1};
#endif

#if defined DITPIN2
VirtualArduinoStyleInputPin ditp2(DITPIN2, VitualArduinoPinPull::VAPP_HIGH);
VirtualPin &vditp2{ditp2};
#endif

#if defined DAHPIN1
VirtualArduinoStyleInputPin dahp1(DAHPIN1, VitualArduinoPinPull::VAPP_HIGH);
VirtualPin &vdahp1{dahp1};
#endif

#if defined DAHPIN2
VirtualArduinoStyleInputPin dahp2(DAHPIN2, VitualArduinoPinPull::VAPP_HIGH);
VirtualPin &vdahp2{dahp2};
#endif

VirtualPinSet ditPaddles;
VirtualPinSet dahPaddles;

#define VIRTUAL_DITS 0
#define VIRTUAL_DAHS 1

void initialize_virtualPins()
{

#if defined DITPIN2
    ditPaddles.pins.push_back(&vditp2);
#endif
#if defined DITPIN1
    ditPaddles.pins.push_back(&vditp1);
#endif
#if defined DAHPIN2
    dahPaddles.pins.push_back(&vdahp2);
#endif
#if defined DAHPIN1
    dahPaddles.pins.push_back(&vdahp1);
#endif

    virtualPins.pinsets.insert(std::make_pair(VIRTUAL_DITS, &ditPaddles));
    virtualPins.pinsets.insert(std::make_pair(VIRTUAL_DAHS, &dahPaddles));
}
#endif