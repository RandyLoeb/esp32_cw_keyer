#ifndef TRANSMITCONTROL_H
#define TRANSMITCONTROL_H
#include <Arduino.h>
#include "keyer_esp32.h"

class TransmitControl
{

public:
    void initialize()
    {
        pinMode(TRANSMIT_PIN, OUTPUT);
        this->unkey();
    }

    void key()
    {
        digitalWrite(TRANSMIT_PIN, HIGH);
    }

    void unkey()
    {
        digitalWrite(TRANSMIT_PIN, LOW);
    }
};

#endif