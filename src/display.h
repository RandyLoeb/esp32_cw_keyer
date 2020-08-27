#define default_display_msg_delay 1000

#ifdef XXX
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

#endif //FEATURE_DISPLAY

