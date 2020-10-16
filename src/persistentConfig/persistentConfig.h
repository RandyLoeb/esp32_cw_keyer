#ifndef PERSISTENTCONFIG_H
#define PERSISTENTCONFIG_H
#include <Arduino.h>
#include <string>
#include <vector>
#include <queue>
#include "ArduinoJson.h"
#define DEFAULT_CONFIG_JSON_DOC_SIZE 4096
//#define PRIMARY_SERIAL_CLS HardwareSerial
class persistentConfig
{

public:
    DynamicJsonDocument configJsonDoc;
    persistentConfig();

    
    String IPAddress;
    String MacAddress;

    virtual void initialize();

    virtual void save();

    virtual void preSave()
    {
        for (std::vector<void (*)()>::iterator it = preSaveCallbacks.begin(); it != preSaveCallbacks.end(); ++it)
        {
            (*it)();
        }
    };

    virtual void postSave()
    {
        for (std::vector<void (*)()>::iterator it = postSaveCallbacks.begin(); it != postSaveCallbacks.end(); ++it)
        {
            (*it)();
        }

        while (!postSaveOneTimeCallbacks.empty())
        {
            void (*oneTime)() = postSaveOneTimeCallbacks.front();
            postSaveOneTimeCallbacks.pop();
            oneTime();
        }
    };

    String
    getJsonStringFromConfiguration();

    byte config_dirty = 0;

    std::vector<void (*)()> wpmChangeCallbacks;

    std::vector<void (*)()> preSaveCallbacks;
    std::vector<void (*)()> postSaveCallbacks;
    std::queue<void (*)()> postSaveOneTimeCallbacks;

    void setWpm(int newWpm, int newWpmFarnsworth, int newWpmFarnsworthSlow)
    {

        //this->configuration.wpm = newWpm;
        this->configJsonDoc["wpm"] = newWpm;

        if (newWpmFarnsworth >= newWpmFarnsworthSlow)
        {
            //this->configuration.wpm_farnsworth = newWpmFarnsworth;
            this->configJsonDoc["wpm_farnsworth"] = newWpmFarnsworth;

            //this->configuration.wpm_farnsworth_slow = newWpmFarnsworthSlow;
            this->configJsonDoc["wpm_farnsworth_slow"] = newWpmFarnsworthSlow;
        }
        for (std::vector<void (*)()>::iterator it = wpmChangeCallbacks.begin(); it != wpmChangeCallbacks.end(); ++it)
        {
            postSaveOneTimeCallbacks.push((*it));
        }
    };

    /* virtual String readFileIntoString(String fileName); */
};
#endif
