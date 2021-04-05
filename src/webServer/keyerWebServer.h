#ifndef KEYERWEBSERVER_H
#define KEYERWEBSERVER_H
#include "ESPAsyncWebServer.h"
#include "SPIFFS.h"
#include "wifiUtils.h"
#include "persistentConfig/persistentConfig.h"
#include "timerStuff/paddlePress.h"
#include "displays/displayCache.h"
#include <queue>
#include <string>
#include <vector>
class KeyerWebServer
{
    //WebServer server;
    AsyncWebServer server;

    AsyncWebSocket ws;

    void initializeServer();

    String getPage(String page);
    String getWifiAdminPage();
    String getWifiAdminCSS();

    String getWifiAdminJS();

public:
    // the web server needs timers off while it handles SPIFFS calls,
    // so we monitor the open SIFFS routes and when they're all done
    // we record that time so some other non-timer loop can detect it
    // and re-enable paddle and other timers

    // make this -1, otherwise millis when the last spiffs completed
    long safeToTurnTimersBackOn = -1;

    // track open spiffs connections
    int _openSpiffs = 0;

    // this gets called right before web server uses SPIFFS
    virtual void preSPIFFS()
    {

        this->safeToTurnTimersBackOn = -1;
        this->_openSpiffs++;
        //Serial.print("open web spiffs:");
        //Serial.println(this->_openSpiffs);

        // we expect callbacks to turn off timers here
        for (std::vector<void (*)()>::iterator it = preSPIFFSCallbacks.begin(); it != preSPIFFSCallbacks.end(); ++it)
        {
            (*it)();
        }
    };

    virtual void postSPIFFS()
    {

        this->_openSpiffs--;
        //Serial.print("open web spiffs:");
        //Serial.println(this->_openSpiffs);
        if (this->_openSpiffs == 0)
        {
            this->safeToTurnTimersBackOn = millis();
        }
    };

    std::vector<void (*)()> preSPIFFSCallbacks;

    //i don't think i use this anymore...
    std::vector<void (*)()> postSPIFFSCallbacks;

    WifiUtils *_wifiUtils;
    std::queue<String> *_textQueue;
    persistentConfig *_persistentConfig;
    DisplayCache *_displayCache;
    KeyerWebServer(WifiUtils *wifiUtils, std::queue<String> *textQueue, persistentConfig *persistentConf, DisplayCache *displayCache);

    void start()
    {
        this->initializeServer();
        this->server.begin();
    }

    void showUtilJson()
    {
        this->_wifiUtils->showJson();
    }
};

#endif