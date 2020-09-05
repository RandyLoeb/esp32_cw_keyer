#ifndef KEYERWEBSERVER_H
#define KEYERWEBSERVER_H
//#include <WebServer.h> //Local WebServer used to serve the configuration portal
#include "ESPAsyncWebServer.h"
#include "SPIFFS.h"
#include "wifiUtils.h"
class KeyerWebServer
{
    //WebServer server;
    AsyncWebServer server;
    //WifiUtils *_wifiUtils;
    void handleRoot();

    void handleRootCss();

    void handleRootJS();

    void handleNotFound();

    void handleReboot();

    void handleWiFiScan();

    void handleUpdateAp();

    void handleForgetAp();

    void initializeServer();

    String getPage(String page);
    String getWifiAdminPage();
    String getWifiAdminCSS();

    String getWifiAdminJS();

public:
    WifiUtils *_wifiUtils;
    KeyerWebServer(WifiUtils *wifiUtils);

    void start()
    {
        this->initializeServer();
        this->server.begin();
    }
    void handleClient();
    void showUtilJson()
    {
        this->_wifiUtils->showJson();
    }
};
#endif