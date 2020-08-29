#ifndef VIRTUALPINS_H
#define VIRTUALPINS_H
#include <map>
#include "virtualPinSet.h"
class VirtualPins
{
public:
    std::map<int, VirtualPinSet *> pinsets;
};
#endif