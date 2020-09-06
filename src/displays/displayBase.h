#ifndef DISPLAYBASE_H
#define DISPLAYBASE_H
#include <Arduino.h>
#include <string>
class displayBase
{

protected:
    void (*sendChar)(char x);

public:
    byte lcd_paddle_echo = 1;
    byte lcd_send_echo = 1;
    virtual int getColsCount() { return 0; };
    virtual int getRowsCount() { return 0; };
    virtual void displayUpdate(char character)
    {
    }

    virtual void displayUpdate(String str)
    {
        for (int i = 0; i < str.length(); i++)
        {
            this->displayUpdate(str.charAt(i));
        }
    }
    virtual void initialize(void (*sendChr)(char x));
    virtual void service_display(){
        //Serial.println("In service_display displayBase...not good...");
    };
    virtual void lcd_center_print_timed(String lcd_print_string, byte row_number, unsigned int duration){
        //Serial.println("In lcd_center_print_timed displayBase...not good...");
    };

    virtual bool isShowingTimedDisplay()
    {
        return false;
    }
};
#endif