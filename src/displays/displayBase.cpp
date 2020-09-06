#include "displayBase.h"
void displayBase::initialize(void (*sendChr)(char x))
{
    this->sendChar = sendChr;

    if (this->getColsCount() < 9)
    {
        this->lcd_center_print_timed("K3NGKeyr", 0, 4000);
    }
    else
    {
        this->lcd_center_print_timed("K3NG Keyer", 0, 4000);
#ifdef OPTION_PERSONALIZED_STARTUP_SCREEN
        if (LCD_ROWS == 2)
        {
#ifdef OPTION_DO_NOT_SAY_HI                                              // if we wish to display the custom field on the second line, we can't say 'hi'
            this->lcd_center_print_timed(custom_startup_field, 1, 4000); // display the custom field on the second line of the display, maximum field length is the number of columns
#endif                                                                   // OPTION_DO_NOT_SAY_HI
        }
        else if (LCD_ROWS > 2)
            this->lcd_center_print_timed(custom_startup_field, 2, 4000); // display the custom field on the third line of the display, maximum field length is the number of columns
#endif                                                                   // OPTION_PERSONALIZED_STARTUP_SCREEN
        /* if (this->getRowsCount() > 3)
            this->lcd_center_print_timed("V: " + String(CODE_VERSION), 3, 4000); // display the code version on the fourth line of the display */
    }

    /* if (keyer_machine_mode != BEACON)
    { */
#ifndef OPTION_DO_NOT_SAY_HI
    // say HI
    // store current setting (compliments of DL2SBA - http://dl2sba.com/ )
    //byte oldKey = key_tx;
    //byte oldSideTone = configControl.configuration.sidetone_mode;
    //key_tx = 0;
    //configControl.configuration.sidetone_mode = SIDETONE_ON;

    this->lcd_center_print_timed("h", 1, 4000);

    //Serial.println("About to say HI...");
    //send_char('H', KEYER_NORMAL);
    this->sendChar('H');

    this->lcd_center_print_timed("hi", 1, 4000);

    //send_char('I', KEYER_NORMAL);
    this->sendChar('I');
    //configControl.configuration.sidetone_mode = oldSideTone;
    //key_tx = oldKey;
#endif //OPTION_DO_NOT_SAY_HI
#ifdef OPTION_BLINK_HI_ON_PTT
    blink_ptt_dits_and_dahs(".... ..");
#endif
    /* } */
}