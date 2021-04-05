#include "FS.h"
#include "SPIFFS.h"
#include "ArduinoJson.h"
#include "spiffsPersistentConfig.h"
#define GENERIC_CHARGRAB

#define CONFIG_JSON_FILENAME "/configuration.json"
#define FORMAT_SPIFFS_IF_FAILED true
#define SPIFFS_LOG_SERIAL false

void SpiffsPersistentConfig::makeSpiffsReady()
{
    if (!SPIFFS.begin(FORMAT_SPIFFS_IF_FAILED))
    {

        /* if (SPIFFS_LOG_SERIAL)
        {
            esp32_port_to_use->println("SPIFFS Mount Failed");
        } */
    }
    else
    {
        /* File root = SPIFFS.open("/");

        File file = root.openNextFile();

        while (file)
        {

            
            if (SPIFFS_LOG_SERIAL)
            {
                esp32_port_to_use->print("FILE: ");
            }
            
            if (SPIFFS_LOG_SERIAL)
            {
                esp32_port_to_use->println(file.name());
            }
            file = root.openNextFile();
        }
 */
        this->_SPIFFS_MADE_READY = 1;
    }
}
int SpiffsPersistentConfig::configFileExists()
{
    if (!_SPIFFS_MADE_READY)
    {
        makeSpiffsReady();
    }
    File file = SPIFFS.open(CONFIG_JSON_FILENAME, FILE_READ);
    int exists = 1;
    if (!file || file.isDirectory())
    {

#ifdef DEBUG_SPIFFS_PERSISTENT_CONFIG_LOG_FILE_EXISTS
        //Serial.println("Config file doesn't exist");
#endif
        exists = 0;
    }
    else
    {
#ifdef DEBUG_SPIFFS_PERSISTENT_CONFIG_LOG_FILE_EXISTS
        //Serial.println("Config file was found.");
#endif
    }
    file.close();
    return exists;
}
String SpiffsPersistentConfig::getConfigFileJsonString()
{
    if (!_SPIFFS_MADE_READY)
    {
        makeSpiffsReady();
    }
#ifdef DEBUG_SPIFFS_PERSISTENT_CONFIG_GET_FILE_JSON
    //ln("About to open config file");
#endif
    File file = SPIFFS.open(CONFIG_JSON_FILENAME, FILE_READ);
#ifdef DEBUG_SPIFFS_PERSISTENT_CONFIG_GET_FILE_JSON
    //Serial.println("config file contents:");
#endif
    String jsonReturn = "";
    while (file.available())
    {
        jsonReturn = jsonReturn + file.readString();
    }
    file.close();
#ifdef DEBUG_SPIFFS_PERSISTENT_CONFIG_GET_FILE_JSON
    //Serial.println(jsonReturn);
#endif
    return jsonReturn;
}
void SpiffsPersistentConfig::setConfigurationFromFile()
{
    String jsonString = getConfigFileJsonString();
#ifdef DEBUG_SPIFFS_PERSISTENT_CONFIG_SET_CONFIG_FROM_FILE
    //Serial.println(jsonString);
#endif

    deserializeJson(this->configJsonDoc, jsonString);
}

void SpiffsPersistentConfig::writeConfigurationToFile()
{
    if (!this->_SPIFFS_MADE_READY)
    {
        this->makeSpiffsReady();
    }
    String jsonToWrite = getJsonStringFromConfiguration();
    File newfile = SPIFFS.open(CONFIG_JSON_FILENAME, FILE_WRITE);
    if (!newfile)
    {
#ifdef DEBUG_SPIFFS_PERSISTENT_CONFIG_WRITE_CONFIG
        //Serial.println("- failed to open file for writing");
#endif
        //return;
    }
    if (newfile.print(jsonToWrite))
    {
#ifdef DEBUG_SPIFFS_PERSISTENT_CONFIG_WRITE_CONFIG
        //Serial.println("- file written");
#endif
        newfile.close();
    }
    else
    {
#ifdef DEBUG_SPIFFS_PERSISTENT_CONFIG_WRITE_CONFIG
        //Serial.println("- write failed");
#endif
    }
    delay(200);
}

void SpiffsPersistentConfig::initializeSpiffs()
{
    //esp32_port_to_use = port_to_use;
}

void SpiffsPersistentConfig::initialize()
{
    //this->esp32_port_to_use = loggingPortToUse;
    //Serial.println("In initialize spiffs persistent");
    if (!configFileExists())
    {
        //Serial.println("did not find config file");
        //barebones to start
        this->configJsonDoc["wpm"] = 20;
        this->configJsonDoc["wpm_farnsworth"] = 20;
        this->configJsonDoc["wpm_farnsworth_slow"] = 20;
        this->configJsonDoc["tone_hz"] = 600;
        writeConfigurationToFile();
    }
    else
    {
        String s = getConfigFileJsonString();
        if (s.length() == 0)
        {
            //Serial.println("config string was empty, initialize");
            writeConfigurationToFile();
        }
    }

    //Serial.println("setting config from file");
    this->configJsonDoc["tone_hz"] = 600;
    setConfigurationFromFile();

    //turn these off to start always
    this->configJsonDoc["tx"] = 0;
    this->configJsonDoc["ws_connect"] = 0;
}
void SpiffsPersistentConfig::save()
{
    this->preSave();
    writeConfigurationToFile();
    this->postSave();
};
