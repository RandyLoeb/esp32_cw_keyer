#ifndef VIRTUALPINS_H
#define VIRTUALPINS_H
#include <map>
#include "virtualPin.h"
class VirtualPins
{
public:
    std::map<int, VirtualPin*> pins;
};
#endif