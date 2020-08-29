#ifndef VIRTUALPINSET_H
#define VIRTUALPINSET_H
#include <vector>
#include "virtualPin.h"
class VirtualPinSet
{
public:
    std::vector<VirtualPin *> pins;
    int digitalRead()
    {
        int ret = 1;
        for (VirtualPin *p : pins)
        {
            ret = p->digitalRead();
            if (ret == 0)
            {
                break;
            }
        }
        return ret;
    };
};
#endif