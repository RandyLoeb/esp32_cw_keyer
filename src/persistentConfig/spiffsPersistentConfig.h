#ifndef SPIFFSPERSISTENTCONFIG_H
#define SPIFFSPERSISTENTCONFIG_H
#include "persistentConfig.h"
#include <Arduino.h>
#include "FS.h"
#include "SPIFFS.h"
#include "ArduinoJson.h"
class SpiffsPersistentConfig : public persistentConfig
{
    int _SPIFFS_MADE_READY = 0;
    PRIMARY_SERIAL_CLS *esp32_port_to_use;
    String getConfigFileJsonString();
    void makeSpiffsReady();
    int configFileExists();
    void writeConfigurationToFile();
    void initializeSpiffs(PRIMARY_SERIAL_CLS *port_to_use);
    void setConfigurationFromFile();

public:
    virtual void initialize(PRIMARY_SERIAL_CLS *loggingPortToUse);

    virtual void save();
};
#endif