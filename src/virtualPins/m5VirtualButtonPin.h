#include "virtualPin.h"
#include <M5Stack.h>
enum M5Btnslist
{
    A,
    B,
    C
};
class M5VirtualButtonPin : public VirtualPin
{
    M5Btnslist btn;
    bool _pressImpliesLow;

public:
    M5VirtualButtonPin(M5Btnslist btn, bool pressImpliesLow = true);

    virtual int digitalRead();
};