#ifndef DISPLAYCACHE_H
#define DISPLAYCACHE_H
#include "ArduinoJson.h"
#include <Arduino.h>
#include <string>

class DisplayCache
{
    String _cache;
    int _limit = 100;

public:
    DisplayCache()
    {
        _cache = "";
    }

    void add(String s)
    {
        _cache += s;
        if (_cache.length() > _limit)
        {

            _cache = _cache.substring(_cache.length() - _limit);
        }
    }

    String getJsonAndClear()
    {
        String ret;
        DynamicJsonDocument jsonDoc(4096);
        jsonDoc["cache"] = _cache;
        _cache = "";
        serializeJson(jsonDoc, ret);

        return ret;
    }
};

#endif