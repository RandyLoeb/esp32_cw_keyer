#ifndef CONVERSIONSTUFF_H
#define CONVERSIONSTUFF_H
#include <Arduino.h>
#include <vector>
#include <string>
#include "cw_utils.h"
#include "displays/keyerDisplay.h"
std::vector<String> conversionQueue;

void convertDitsDahsToCharsAndSpaces(String ditDahOrSpace)
{
    if (ditDahOrSpace != "charspace")
    {
        //just push and wait for a space
        conversionQueue.push_back(ditDahOrSpace);
    }
    else
    {
        long characterCode = 0;
        //we got a space, time to analyze
        for (std::vector<String>::iterator it = conversionQueue.begin(); it != conversionQueue.end(); ++it)
        {
            if (*it != "charspace")
            {
                characterCode *= 10;
                if (*it == "dit")
                {
                    characterCode += 1;
                }
                if (*it == "dah")
                {
                    characterCode += 2;
                }
            }
        }

        conversionQueue.clear();
        //td::cout << ' ' << *it;
        //    std::cout << '\n';

        String cwCharacter = convert_cw_number_to_string(characterCode);
        displayControl.displayUpdate(cwCharacter);
    }
}

#endif