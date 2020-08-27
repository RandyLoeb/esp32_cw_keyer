class displayBase
{

public:
    byte lcd_paddle_echo = 1;
    byte lcd_send_echo = 1;
    virtual int getColsCount() { return 0; };
    virtual int getRowsCount() { return 0; };
    virtual void displayUpdate(char character)
    {
    }

    virtual void initialize()
    {
        Serial.println("initialize in displayBase...not good...");
    };
    virtual void service_display()
    {
        //Serial.println("In service_display displayBase...not good...");
    };
    virtual void lcd_center_print_timed(String lcd_print_string, byte row_number, unsigned int duration)
    {
        Serial.println("In lcd_center_print_timed displayBase...not good...");
    };
};