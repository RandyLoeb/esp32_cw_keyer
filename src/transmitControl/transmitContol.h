#ifndef TRANSMITCONTROL_H
#define TRANSMITCONTROL_H
#include <Arduino.h>
#include "keyer_esp32.h"
#include "persistentConfig/persistentConfig.h"

class TransmitControl
{
    persistentConfig *_config;

public:
    void initialize(persistentConfig *config)
    {
        pinMode(TRANSMIT_PIN, OUTPUT);
        this->unkey(true);
        this->_config = config;
    }

    void key()
    {
        if (_config->configuration.tx > 0)
        {
            digitalWrite(TRANSMIT_PIN, HIGH);
        }
    }

    void unkey(bool force = false)
    {
        if (_config->configuration.tx > 0 || force)
        {
            digitalWrite(TRANSMIT_PIN, LOW);
        }
    }
};

#endif