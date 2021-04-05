#include <Arduino.h>
#define PRIMARY_SERIAL_CLS HardwareSerial
#include "persistentConfig.h"
#include "ArduinoJson.h"
#include "keyer_settings.h"
#include "keyer.h"
persistentConfig::persistentConfig() : configJsonDoc(DEFAULT_CONFIG_JSON_DOC_SIZE)
{
}

void persistentConfig::initialize()
{
}

void persistentConfig::save()
{
    //Serial.println("in base save...bad!");
};

String
persistentConfig::getJsonStringFromConfiguration()
{

    String returnString;
    serializeJson(this->configJsonDoc, returnString);

    return returnString;
}
