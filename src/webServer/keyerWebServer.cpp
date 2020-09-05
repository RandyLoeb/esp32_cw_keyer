#include <WebServer.h> //Local WebServer used to serve the configuration portal
#include "SPIFFS.h"
#include "wifiUtils.h"
#include "keyerWebServer.h"

KeyerWebServer::KeyerWebServer(WifiUtils *wifiUtils) : server(80)
{
    Serial.println("got addr of wifi utils");
    //Serial.println(wifiUtils);
    this->_wifiUtils = wifiUtils;
};

void KeyerWebServer::handleRoot()
{
    Serial.println("hadnling root...");
    this->_wifiUtils->showJson();
    //_MS_SINCE_LAST_WEB = 0;
    server.send(200, "text/html", getWifiAdminPage()); // Send HTTP status 200 (Ok) and send some text to the browser/client
}

void KeyerWebServer::handleRootCss()
{
    //_MS_SINCE_LAST_WEB = 0;
    server.send(200, "text/css", getWifiAdminCSS());
}

void KeyerWebServer::handleRootJS()
{
    //_MS_SINCE_LAST_WEB = 0;
    server.send(200, "text/javascript", getWifiAdminJS());
}

void KeyerWebServer::handleNotFound()
{
    //_MS_SINCE_LAST_WEB = 0;
    Serial.println("Not found reached.");
    server.sendHeader("Location", "http://" + WiFi.softAPIP().toString() + "/");
    server.send(302, "text/plain", "");
    server.client().stop();
}

void KeyerWebServer::handleReboot()
{
    //_MS_SINCE_LAST_WEB = 0;
    server.send(200, "text/html", "rebooting");
    ESP.restart();
}

void KeyerWebServer::handleWiFiScan()
{
    String scanJson = this->_wifiUtils->getWiFiScan();
    server.send(200, "application/json", scanJson);
}

void KeyerWebServer::handleUpdateAp()
{
    Serial.println(server.arg(0));
    this->_wifiUtils->updateAp(server.arg(0));
    server.send(200, "text/html", "ok");
}

void KeyerWebServer::handleForgetAp()
{
    this->_wifiUtils->forgetAp(server.arg(0));
    server.send(200, "text/html", "ok");
}

void KeyerWebServer::initializeServer()
{
    server.enableCORS(true);
    server.enableCrossOrigin(true);
    //WifiUtils * myutils = this->_wifiUtils;
    server.on("/", [this]() { this->handleRoot(); }); // Call the 'handleRoot' function when a client requests URI "/"
    server.on("/wifiadmin.html", [this]() { this->handleRoot(); });
    server.on("/wifiadmin.css", [this]() { this->handleRootCss(); });
    server.on("/wifiadmin.js", [this]() { this->handleRootJS(); });

    server.on("/wifiscan", [this]() { this->handleWiFiScan(); });
    server.on("/updateap", [this]() { this->handleUpdateAp(); });
    server.on("/forgetap", [this]() { this->handleForgetAp(); });
    server.on("/reboot", [this]() { this->handleReboot(); });
    server.onNotFound([this]() { this->handleNotFound(); });
}

String KeyerWebServer::getPage(String page)
{
    File f = SPIFFS.open(page, FILE_READ);
    String pageBody = "";
    while (f.available())
    {
        pageBody = pageBody + f.readString();
    }
    f.close();
    //Serial.println(pageBody);
    return pageBody;
}
String KeyerWebServer::getWifiAdminPage()
{
    return getPage("/wifiadmin.html");
}

String KeyerWebServer::getWifiAdminCSS()
{
    return getPage("/wifiadmin.css");
}

String KeyerWebServer::getWifiAdminJS()
{
    return getPage("/wifiadmin.js");
}

void KeyerWebServer::handleClient()
{
    this->server.handleClient();
}