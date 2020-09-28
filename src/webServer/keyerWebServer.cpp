//#include <WebServer.h> //Local WebServer used to serve the configuration portal
#include "ESPAsyncWebServer.h"
#include "SPIFFS.h"
#include "wifiUtils.h"
#include "persistentConfig/persistentConfig.h"
#include "keyerWebServer.h"
#include <queue>
#include <string>
#include "timerStuff/paddlePress.h"
#include "keyer_esp32.h"

void (*_ditdahCallBack)(DitOrDah ditOrDah);
std::queue<String> *txtQueue;

#if defined REMOTE_KEYER
void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len)
{
    // handling code
    if (type == WS_EVT_CONNECT)
    {
        Serial.println("ws connected");
        //client connected
        //os_printf("ws[%s][%u] connect\n", server->url(), client->id());
        client->printf("Hello Client %u :)", client->id());
        client->ping();
    }
    else if (type == WS_EVT_DISCONNECT)
    {
        Serial.println("ws disconnected");
        //client disconnected
        //os_printf("ws[%s][%u] disconnect: %u\n", server->url(), client->id());
    }
    else if (type == WS_EVT_ERROR)
    {
        Serial.println("ws error");
        //error was received from the other end
        //os_printf("ws[%s][%u] error(%u): %s\n", server->url(), client->id(), *((uint16_t *)arg), (char *)data);
    }
    else if (type == WS_EVT_PONG)
    {
        Serial.println("ws pong");
        //pong message was received (in response to a ping request maybe)
        //os_printf("ws[%s][%u] pong[%u]: %s\n", server->url(), client->id(), len, (len) ? (char *)data : "");
    }
    else if (type == WS_EVT_DATA)
    {
        //Serial.println("ws got data");
        //data packet
        AwsFrameInfo *info = (AwsFrameInfo *)arg;
        if (info->final && info->index == 0 && info->len == len)
        {
            //the whole message is in a single frame and we got all of it's data
            //os_printf("ws[%s][%u] %s-message[%llu]: ", server->url(), client->id(), (info->opcode == WS_TEXT) ? "text" : "binary", info->len);
            if (info->opcode == WS_TEXT)
            {
                data[len] = 0;
                String myData = String((char *)data);
                //Serial.println(myData);
                if (myData == "dit" || myData == "dah")
                {
                    _ditdahCallBack(myData == "dit" ? DitOrDah::DIT : DitOrDah::DAH);
                }
                else
                {
                    txtQueue->push(myData);
                }

                //os_printf("%s\n", (char *)data);
            }
            else
            {
                String mydata = "";
                for (size_t i = 0; i < info->len; i++)
                {
                    //os_printf("%02x ", data[i]);
                    mydata += data[i];
                }
                Serial.println(mydata);
                //os_printf("\n");
            }
            /* if (info->opcode == WS_TEXT)
                client->text("I got your text message");
            else
                client->binary("I got your binary message"); */
        }
        else
        {
            //message is comprised of multiple frames or the frame is split into multiple packets
            if (info->index == 0)
            {
                if (info->num == 0)
                {
                    //os_printf("ws[%s][%u] %s-message start\n", server->url(), client->id(), (info->message_opcode == WS_TEXT) ? "text" : "binary");
                    //os_printf("ws[%s][%u] frame[%u] start[%llu]\n", server->url(), client->id(), info->num, info->len);
                }
            }

            //os_printf("ws[%s][%u] frame[%u] %s[%llu - %llu]: ", server->url(), client->id(), info->num, (info->message_opcode == WS_TEXT) ? "text" : "binary", info->index, info->index + len);
            if (info->message_opcode == WS_TEXT)
            {
                data[len] = 0;
                //os_printf("%s\n", (char *)data);
            }
            else
            {
                for (size_t i = 0; i < len; i++)
                {
                    //os_printf("%02x ", data[i]);
                }
                //os_printf("\n");
            }

            if ((info->index + len) == info->len)
            {
                //os_printf("ws[%s][%u] frame[%u] end[%llu]\n", server->url(), client->id(), info->num, info->len);
                if (info->final)
                {
                    //os_printf("ws[%s][%u] %s-message end\n", server->url(), client->id(), (info->message_opcode == WS_TEXT) ? "text" : "binary");
                    /* if (info->message_opcode == WS_TEXT)
                        client->text("I got your text message");
                    else
                        client->binary("I got your binary message"); */
                }
            }
        }
    }
}
#endif

KeyerWebServer::KeyerWebServer(WifiUtils *wifiUtils, std::queue<String> *textQueue, persistentConfig *persistentConf, void (*ditCallback)(),
                               void (*ditdahCB)(DitOrDah ditOrDah)) : server(80), ws("/ws")
{
    Serial.println("got addr of wifi utils");
    //Serial.println(wifiUtils);
    this->_wifiUtils = wifiUtils;
    this->_textQueue = textQueue;
    txtQueue = textQueue;
    this->_persistentConfig = persistentConf;
    this->ditCallbck = ditCallback;
    _ditdahCallBack = ditdahCB;
#if defined REMOTE_KEYER
    Serial.println("REMOTE_KEYER ok");
    this->ws.onEvent(onWsEvent);
    this->server.addHandler(&this->ws);
#endif
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
        this->preSPIFFS();
        request->onDisconnect([this]() { this->postSPIFFS(); });

        //request->send(200, "text/html", "ok");
        request->send(SPIFFS, "/keyer.html", "text/html");
        //this->postSPIFFS();
    });

    server.on("/jquery-3.5.1.min.js", HTTP_GET, [this](AsyncWebServerRequest *request) {
        this->preSPIFFS();
        request->onDisconnect([this]() { this->postSPIFFS(); });
        request->send(SPIFFS, "/jquery-3.5.1.min.js", "text/javascript");
        //this->postSPIFFS();
    });
    server.on("/knockout-3.5.1.min.js", HTTP_GET, [this](AsyncWebServerRequest *request) {
        this->preSPIFFS();
        request->onDisconnect([this]() { this->postSPIFFS(); });
        request->send(SPIFFS, "/knockout-3.5.1.min.js", "text/javascript");
        //this->postSPIFFS();
    });
    server.on("/axios.min.js", HTTP_GET, [this](AsyncWebServerRequest *request) {
        this->preSPIFFS();
        request->onDisconnect([this]() { this->postSPIFFS(); });
        request->send(SPIFFS, "/axios.min.js", "text/javascript");
        //this->postSPIFFS();
    });
    server.on("/keyer.js", HTTP_GET, [this](AsyncWebServerRequest *request) {
        this->preSPIFFS();
        request->onDisconnect([this]() { this->postSPIFFS(); });
        request->send(SPIFFS, "/keyer.js", "text/javascript");
        //this->postSPIFFS();
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

    server.on("/getconfig", [this](AsyncWebServerRequest *request) {
        String scanJson = this->_persistentConfig->getJsonStringFromConfiguration();
        request->send(200, "application/json", scanJson); });

    server.on(
        "/setconfig", HTTP_POST, [this](AsyncWebServerRequest *request) {},
        NULL, [this](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
        String jsonIn = String((const char*)data);
        Serial.println(jsonIn);
        DynamicJsonDocument jBuffer(4096);
        deserializeJson(jBuffer, jsonIn);
        bool debugMatched = false;
        JsonArray settingsAry = jBuffer["settings"].as<JsonArray>();
        for (JsonObject settingsObj : settingsAry) {
            String settingName = settingsObj["name"].as<String>();
            
            if (settingName.startsWith("wpm"))
            {
                int wpm = _persistentConfig->configJsonDoc["wpm"].as<int>();
                int wpmf = _persistentConfig->configJsonDoc["wpm_farnsworth"].as<int>();
                int wpmfs = _persistentConfig->configJsonDoc["wpm_farnsworth_slow"].as<int>();
                if (settingName=="wpm")
                {
                    wpm=settingsObj["value"].as<int>();
                    debugMatched=true;
                }
                if (settingName=="wpm_farnsworth")
                {
                    wpmf=settingsObj["value"].as<int>();
                    debugMatched=true;
                }
                if (settingName=="wpm_farnsworth_slow")
                {
                    wpmfs=settingsObj["value"].as<int>();
                    debugMatched=true;
                }

                if (wpmf>=wpmfs) 
                {
                    this->_persistentConfig->setWpm(wpm,wpmf,wpmfs);
                }
            }

            if (settingName == "tx")
            {
                this->_persistentConfig->configJsonDoc["tx"] = settingsObj["value"].as<int>();
            }

            if (settingName == "ws_connect")
            {
                this->_persistentConfig->configJsonDoc["ws_connect"] = settingsObj["value"].as<int>();
            }

            if (settingName == "ws_ip")
            {
                this->_persistentConfig->configJsonDoc["ws_ip"] = settingsObj["value"].as<String>();
            }

        }
        Serial.print("settings matched?"); Serial.println(debugMatched);

        this->_persistentConfig->config_dirty=1;
        //send the config
        String scanJson = this->_persistentConfig->getJsonStringFromConfiguration();
        request->send(200, "application/json", scanJson); });

    server.on("/dit", [this](AsyncWebServerRequest *request) {
        Serial.println("got dit!");
        this->ditCallbck();
        request->send(200, "text/html", "");
    });
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