
#include <string>
#include "displayBase.h"
class M5KeyerDisplay : public displayBase
{
    String displayContents = "";
    int _DISPLAY_INITIALIZED = 0;
    int _DISPLAY_HEIGHT = 0;
    int _DISPLAY_WIDTH = 0;

    String timedDisplayLine1 = "";
    String lastTimedDisplayLine1 = "";
    String timedDisplayLine2 = "";
    String lastTimedDisplayLine2 = "";
    int timedDelay = 0;
    int millisDelayStarted = 0;
    bool delayMode = false;

public:
    virtual void initialize();
    virtual void displayUpdate(char character);
    virtual void service_display();
    virtual void lcd_center_print_timed(String lcd_print_string, byte row_number, unsigned int duration);
    virtual int getColsCount();
    virtual int getRowsCount();
};