
#include "displayBase.h"
#include <LiquidCrystal_I2C.h>

class lcdDisplay : public displayBase
{

    enum lcd_statuses
    {
        LCD_CLEAR,
        LCD_REVERT,
        LCD_TIMED_MESSAGE,
        LCD_SCROLL_MSG
    };

    byte lcd_status = LCD_CLEAR;
    unsigned long lcd_timed_message_clear_time = 0;
    byte lcd_previous_status = LCD_CLEAR;
    byte lcd_scroll_buffer_dirty = 0;
    String lcd_scroll_buffer[LCD_ROWS];
    byte lcd_scroll_flag = 0;

    LiquidCrystal_I2C lcd;
    void lcd_clear()
    {
        lcd.clear();
        lcd.noCursor(); //sp5iou 20180328
        lcd_status = LCD_CLEAR;
    }

    void display_scroll_print_char(char charin)
    {

        static byte column_pointer = 0;
        static byte row_pointer = 0;
        static byte holding_space = 0;
        byte x = 0;

#ifdef DEBUG_DISPLAY_SCROLL_PRINT_CHAR
        debug_serial_port->print(F("display_scroll_print_char: "));
        debug_serial_port->write(charin);
        debug_serial_port->print(" ");
        debug_serial_port->println(charin);
#endif //DEBUG_DISPLAY_SCROLL_PRINT_CHAR

#ifdef OPTION_DISPLAY_NON_ENGLISH_EXTENSIONS
        switch (byte(charin))
        {
        case 220:
            charin = 0;
            break; // U_umlaut  (D, ...)
        case 214:
            charin = 1;
            break; // O_umlaut  (D, SM, OH, ...)
        case 196:
            charin = 2;
            break; // A_umlaut  (D, SM, OH, ...)
        case 198:
            charin = 3;
            break; // AE_capital (OZ, LA)
        case 216:
            charin = 4;
            break; // OE_capital (OZ, LA)
        case 197:
            charin = 6;
            break; // AA_capital (OZ, LA, SM)
        case 209:
            charin = 7;
            break; // N-tilde (EA)
        }
#endif //OPTION_DISPLAY_NON_ENGLISH_EXTENSIONS

        if (lcd_status != LCD_SCROLL_MSG)
        {
            lcd_status = LCD_SCROLL_MSG;
            lcd.clear();
        }

        if (charin == ' ')
        {
            holding_space = 1;
            return;
        }

        if (holding_space)
        { // ok, I admit this is a hack.  Hold on to spaces and don't scroll until the next char comes in...
            if (column_pointer > (LCD_COLUMNS - 1))
            {
                row_pointer++;
                column_pointer = 0;
                if (row_pointer > (LCD_ROWS - 1))
                {
                    for (x = 0; x < (LCD_ROWS - 1); x++)
                    {
                        lcd_scroll_buffer[x] = lcd_scroll_buffer[x + 1];
                    }
                    lcd_scroll_buffer[x] = "";
                    row_pointer--;
                    lcd_scroll_flag = 1;
                }
            }
            if (column_pointer > 0)
            { // don't put a space in the first column
                lcd_scroll_buffer[row_pointer].concat(' ');
                column_pointer++;
            }
            holding_space = 0;
        }

        if (column_pointer > (LCD_COLUMNS - 1))
        {
            row_pointer++;
            column_pointer = 0;
            if (row_pointer > (LCD_ROWS - 1))
            {
                for (x = 0; x < (LCD_ROWS - 1); x++)
                {
                    lcd_scroll_buffer[x] = lcd_scroll_buffer[x + 1];
                }
                lcd_scroll_buffer[x] = "";
                row_pointer--;
                lcd_scroll_flag = 1;
            }
        }
        lcd_scroll_buffer[row_pointer].concat(charin);
        column_pointer++;

        lcd_scroll_buffer_dirty = 1;
    }

public:
    lcdDisplay() : lcd(0x27, 16, 2){};

    void service_display()
    {

#define SCREEN_REFRESH_IDLE 0
#define SCREEN_REFRESH_INIT 1
#define SCREEN_REFRESH_IN_PROGRESS 2

        static byte x = 0;
        static byte y = 0;

        static byte screen_refresh_status = SCREEN_REFRESH_IDLE;

        if (screen_refresh_status == SCREEN_REFRESH_INIT)
        {
            lcd.setCursor(0, 0);
            y = 0;
            x = 0;
            screen_refresh_status = SCREEN_REFRESH_IN_PROGRESS;
            return;
        }

        if (screen_refresh_status == SCREEN_REFRESH_IN_PROGRESS)
        {
            if (x > lcd_scroll_buffer[y].length())
            {
                y++;
                if (y >= LCD_ROWS)
                {
                    screen_refresh_status = SCREEN_REFRESH_IDLE;
                    lcd_scroll_buffer_dirty = 0;
                    return;
                }
                else
                {
                    x = 0;
                    lcd.setCursor(0, y);
                }
            }
            else
            {
                if (lcd_scroll_buffer[y].charAt(x) > 0)
                {
                    lcd.print(lcd_scroll_buffer[y].charAt(x));
                }
                x++;
            }
        }

        if (screen_refresh_status == SCREEN_REFRESH_IDLE)
        {
            if (lcd_status == LCD_REVERT)
            {
                lcd_status = lcd_previous_status;
                switch (lcd_status)
                {
                case LCD_CLEAR:
                    lcd_clear();
                    break;
                case LCD_SCROLL_MSG:
                    lcd.clear();
                    // for (x = 0;x < LCD_ROWS;x++){
                    //   //clear_display_row(x);
                    //   lcd.setCursor(0,x);
                    //   lcd.print(lcd_scroll_buffer[x]);
                    // }
                    screen_refresh_status = SCREEN_REFRESH_INIT;
                    lcd_scroll_flag = 0;
                    //lcd_scroll_buffer_dirty = 0;
                    break;
                }
            }
            else
            {
                switch (lcd_status)
                {
                case LCD_CLEAR:
                    break;
                case LCD_TIMED_MESSAGE:
                    if (millis() > lcd_timed_message_clear_time)
                    {
                        lcd_status = LCD_REVERT;
                    }
                case LCD_SCROLL_MSG:
                    if (lcd_scroll_buffer_dirty)
                    {
                        if (lcd_scroll_flag)
                        {
                            lcd.clear();
                            lcd_scroll_flag = 0;
                        }
                        // for (x = 0;x < LCD_ROWS;x++){
                        //   //clear_display_row(x);
                        //   lcd.setCursor(0,x);
                        //   lcd.print(lcd_scroll_buffer[x]);
                        // }
                        //lcd_scroll_buffer_dirty = 0;
                        screen_refresh_status = SCREEN_REFRESH_INIT;
                    }
                    break;
                }
            }
        }
    }

    void lcd_center_print_timed(String lcd_print_string, byte row_number, unsigned int duration)
    {
        lcd.noCursor(); //sp5iou 20180328
        if (lcd_status != LCD_TIMED_MESSAGE)
        {
            lcd_previous_status = lcd_status;
            lcd_status = LCD_TIMED_MESSAGE;
            lcd.clear();
        }
        else
        {
            clear_display_row(row_number);
        }
        lcd.setCursor(((LCD_COLUMNS - lcd_print_string.length()) / 2), row_number);
        lcd.print(lcd_print_string);
        lcd_timed_message_clear_time = millis() + duration;
    }

    void displayUpdate(char character)
    {
        display_scroll_print_char(character);
    }

    void initialize()
    {
        //lcd.begin(LCD_COLUMNS, LCD_ROWS);
        // initialize LCD
        Wire.begin(I2C_SDA, I2C_SCL);
        lcd.init();
        // turn on LCD backlight
        lcd.backlight();
    }

    int getColsCount()
    {
        return 16;
    }

    int getRowsCount()
    {
        return 2;
    }
};