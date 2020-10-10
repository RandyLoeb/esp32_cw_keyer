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
        this->_config = config;
        this->unkey(true);
        

        Serial.print("checking this _config in transmit control:");
        Serial.println(this->_config != nullptr);
    }

    void key()
    {
        if (this->_config->configJsonDoc["tx"].as<int>() > 0)
        {
            digitalWrite(TRANSMIT_PIN, HIGH);
        }
    }

    void unkey(bool force = false)
    {
        if (this->_config->configJsonDoc["tx"].as<int>() > 0 || force)
        {
            digitalWrite(TRANSMIT_PIN, LOW);
        }
    }
};

#endif