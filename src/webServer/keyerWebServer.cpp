//#include <WebServer.h> //Local WebServer used to serve the configuration portal
#include "ESPAsyncWebServer.h"
#include "SPIFFS.h"
#include "wifiUtils.h"
#include "keyerWebServer.h"
#include <queue>
#include <string>

KeyerWebServer::KeyerWebServer(WifiUtils *wifiUtils, std::queue<String> *textQueue) : server(80)
{
    Serial.println("got addr of wifi utils");
    //Serial.println(wifiUtils);
    this->_wifiUtils = wifiUtils;
    this->_textQueue = textQueue;
};

void KeyerWebServer::handleRoot()
{
    Serial.println("hadnling root...");
    //this->_wifiUtils->showJson();
    //_MS_SINCE_LAST_WEB = 0;
    //server.send(200, "text/html", getWifiAdminPage()); // Send HTTP status 200 (Ok) and send some text to the browser/client
}

void KeyerWebServer::handleRootCss()
{
    //_MS_SINCE_LAST_WEB = 0;
    //server.send(200, "text/css", getWifiAdminCSS());
}

void KeyerWebServer::handleRootJS()
{
    //_MS_SINCE_LAST_WEB = 0;
    //server.send(200, "text/javascript", getWifiAdminJS());
}

void KeyerWebServer::handleNotFound()
{
    //_MS_SINCE_LAST_WEB = 0;
    Serial.println("Not found reached.");
    //server.sendHeader("Location", "http://" + WiFi.softAPIP().toString() + "/");
    //server.send(302, "text/plain", "");
    //server.client().stop();
}

void KeyerWebServer::handleReboot()
{
    //_MS_SINCE_LAST_WEB = 0;
    //server.send(200, "text/html", "rebooting");
    //ESP.restart();
}

void KeyerWebServer::handleWiFiScan()
{
    //String scanJson = this->_wifiUtils->getWiFiScan();
    //server.send(200, "application/json", scanJson);
}

void KeyerWebServer::handleUpdateAp()
{
    //Serial.println(server.arg(0));
    //this->_wifiUtils->updateAp(server.arg(0));
    //server.send(200, "text/html", "ok");
}

void KeyerWebServer::handleForgetAp()
{
    //this->_wifiUtils->forgetAp(server.arg(0));
    //server.send(200, "text/html", "ok");
}

void KeyerWebServer::initializeServer()
{
    //server.enableCORS(true);
    //server.enableCrossOrigin(true);
    //WifiUtils * myutils = this->_wifiUtils;
    server.on("/", HTTP_GET, [this](AsyncWebServerRequest *request) { request->send(SPIFFS, "/wifiadmin.html", "text/html"); }); // Call the 'handleRoot' function when a client requests URI "/"
    server.on("/wifiadmin.html", HTTP_GET, [this](AsyncWebServerRequest *request) { request->send(SPIFFS, "/wifiadmin.html", "text/html"); });
    server.on("/wifiadmin.css", HTTP_GET, [this](AsyncWebServerRequest *request) { request->send(SPIFFS, "/wifiadmin.css", "text/css"); });
    server.on("/wifiadmin.js", HTTP_GET, [this](AsyncWebServerRequest *request) { request->send(SPIFFS, "/wifiadmin.js", "text/javascript"); });

    server.on("/wifiscan", [this](AsyncWebServerRequest *request) {
        String scanJson = this->_wifiUtils->getWiFiScan();
        request->send(200, "application/json", scanJson); });

    //nothing in first lambda blocks, and dont remove it
    server.on(
        "/updateap", HTTP_POST, [this](AsyncWebServerRequest *request) {}, NULL, [this](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
     String s = String((const char*)data);
  
      this->_wifiUtils->updateAp(s);
        request->send(200, "text/html", "ok"); });

    server.on(
        "/forgetap", HTTP_POST, [this](AsyncWebServerRequest *request) {}, NULL, [this](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
     String s = String((const char*)data);
  
      this->_wifiUtils->forgetAp(s);
        request->send(200, "text/html", "ok"); });
    server.on("/reboot", [this](AsyncWebServerRequest *request) {
         request->send(200, "text/html", "rebooting");
        ESP.restart(); });
    server.on("/keyer.html", HTTP_GET, [this](AsyncWebServerRequest *request) {
        request->send(SPIFFS, "/keyer.html", "text/html");
    });

    server.on("/jquery-3.5.1.min.js", HTTP_GET, [this](AsyncWebServerRequest *request) {
        request->send(SPIFFS, "/jquery-3.5.1.min.js", "text/javascript");
    });
    server.on("/knockout-3.5.1.min.js", HTTP_GET, [this](AsyncWebServerRequest *request) {
        request->send(SPIFFS, "/knockout-3.5.1.min.js", "text/javascript");
    });
    server.on("/axios.min.js", HTTP_GET, [this](AsyncWebServerRequest *request) {
        request->send(SPIFFS, "/axios.min.js", "text/javascript");
    });
    server.on("/keyer.js", HTTP_GET, [this](AsyncWebServerRequest *request) {
        request->send(SPIFFS, "/keyer.js", "text/javascript");
    });

    server.on(
        "/textsubmit", HTTP_POST, [this](AsyncWebServerRequest *request) {},
        NULL, [this](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
        String jsonIn = String((const char*)data);
        DynamicJsonDocument jBuffer(4096);
        deserializeJson(jBuffer, jsonIn);

        String textIn = jBuffer["text"];
        this->_textQueue->push(textIn);
        request->send(200, "text/html", "ok"); });

    server.onNotFound([this](AsyncWebServerRequest *request) { request->redirect("http://" + WiFi.softAPIP().toString() + "/"); });
}

String KeyerWebServer::getPage(String page)
{
    String pageBody = "";
    File f = SPIFFS.open(page, FILE_READ);
    if (!f)
    {
        Serial.println("problem opening " + page);
    }
    else
    {

        while (f.available())
        {
            pageBody = pageBody + f.readString();
        }
        f.close();
        //Serial.println(pageBody);
    }
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
    //this->server.handleClient();
}