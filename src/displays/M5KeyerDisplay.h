
#include <string>
#include "displayBase.h"
class M5KeyerDisplay : public displayBase
{

public:
    virtual void initialize();
    virtual void displayUpdate(char character);
    virtual void service_display();
    virtual void lcd_center_print_timed(String lcd_print_string, byte row_number, unsigned int duration);
    virtual int getColsCount();
    virtual int getRowsCount();
};