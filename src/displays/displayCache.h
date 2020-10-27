#ifndef DISPLAYCACHE_H
#define DISPLAYCACHE_H
#include "ArduinoJson.h"
#include <Arduino.h>
#include <string>
#include <iterator>
#include <map>

class DisplayCache
{
    //String _cache;
    String _cacheBuffer;
    int _limit = 100;
    bool _needFlush;
    bool _reading;
    String _lastRead;
    int _counter;
    std::map<int, String> _cache;

public:
    DisplayCache()
    {
        //this->_cache = "";
        this->_needFlush = false;
        this->_reading = false;
        this->_cacheBuffer = "";
        this->_counter = 0;
    }

    void add(String s)
    {
        this->_cache.insert(std::pair<int, String>(this->_counter++, String(s)));
        if (this->_cache.size() > this->_limit)
        {
            this->_cache.erase(this->_cache.begin(), this->_cache.find(this->_counter - this->_limit));
        }
        /*
        if (this->_reading)
        {
            this->_cacheBuffer += s;
        }
        else
        {
            this->_cache += this->_cacheBuffer + s;
            this->_cacheBuffer = "";
            if (this->_cache.length() > this->_limit)
            {

                this->_cache = this->_cache.substring(this->_cache.length() - _limit);
            }
        }
        */
    }

    String getJsonAndClear(int lastPollIndex)
    {
        this->_reading = true;
        String ret;
        String cacheText = "";
        DynamicJsonDocument jsonDoc(4096);
        std::map<int, String>::iterator itr;
        int lastIndex = lastPollIndex;

        for (itr = this->_cache.begin(); itr != this->_cache.end(); ++itr)
        {
            if (itr->first > lastPollIndex)
            {
                cacheText += itr->second;
                lastIndex = itr->first;
            }
        }
        this->_cache.erase(this->_cache.begin(), this->_cache.find(lastIndex));
        jsonDoc["cache"] = String(cacheText);
        jsonDoc["lastPollIndex"] = lastIndex;

        this->_reading = false;

        serializeJson(jsonDoc, ret);

        return ret;
    }
};

#endif