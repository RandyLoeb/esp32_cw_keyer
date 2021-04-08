#ifndef WEBSOCKETSINIT_H
#define WEBSOCKETSINIT_H
#include <timerStuff/timerStuff.h>
#if !defined REMOTE_KEYER

#include <ArduinoWebsockets.h>
//https://github.com/gilmaimon/ArduinoWebsockets?utm_source=platformio&utm_medium=piohome
using namespace websockets;
WebsocketsClient client;
bool clientIntiialized = false;

void connectWsClient()
{
    if (_timerStuffConfig->configJsonDoc["ws_connect"].as<int>() == 1 && _timerStuffConfig->configJsonDoc["ws_ip"].as<String>().length() > 0)
    {
        // Connect to server
        client.connect(_timerStuffConfig->configJsonDoc["ws_ip"].as<String>(), 80, "/ws");
    }
}

void onMessageCallback(WebsocketsMessage message)
{
    //Serial.print("Got Message: ");
    //Serial.println(message.data());
}

void onEventsCallback(WebsocketsEvent event, String data)
{
    if (event == WebsocketsEvent::ConnectionOpened)
    {
        //Serial.println("Connnection Opened");
        clientIntiialized = true;
    }
    else if (event == WebsocketsEvent::ConnectionClosed)
    {
        //Serial.println("Connnection Closed");
        clientIntiialized = false;
    }
    else if (event == WebsocketsEvent::GotPing)
    {
        //Serial.println("Got a Ping!");
    }
    else if (event == WebsocketsEvent::GotPong)
    {
        //Serial.println("Got a Pong!");
    }
}

#endif
#endif