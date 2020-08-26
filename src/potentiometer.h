#include "display.h"
byte pot_wpm_low_value;

#ifdef FEATURE_POTENTIOMETER
byte pot_wpm_high_value;
byte last_pot_wpm_read;
int pot_full_scale_reading = default_pot_full_scale_reading;
#endif //FEATURE_POTENTIOMETER

#ifdef FEATURE_POTENTIOMETER
byte pot_value_wpm()
{
    // int pot_read = analogRead(potentiometer);
    // byte return_value = map(pot_read, 0, pot_full_scale_reading, pot_wpm_low_value, pot_wpm_high_value);
    // return return_value;

    static int last_pot_read = 0;
    static byte return_value = 0;
    int pot_read = analogRead(potentiometer);
    if (abs(pot_read - last_pot_read) > potentiometer_reading_threshold)
    {
        return_value = map(pot_read, 0, pot_full_scale_reading, pot_wpm_low_value, pot_wpm_high_value);
        last_pot_read = pot_read;
    }
    return return_value;
}

#endif
void speed_set(int wpm_set)
{

    if ((wpm_set > 0) && (wpm_set < 1000))
    {
        configuration.wpm = wpm_set;
        config_dirty = 1;

#ifdef FEATURE_DYNAMIC_DAH_TO_DIT_RATIO
        if ((configuration.wpm >= DYNAMIC_DAH_TO_DIT_RATIO_LOWER_LIMIT_WPM) && (configuration.wpm <= DYNAMIC_DAH_TO_DIT_RATIO_UPPER_LIMIT_WPM))
        {
            int dynamicweightvalue = map(configuration.wpm, DYNAMIC_DAH_TO_DIT_RATIO_LOWER_LIMIT_WPM, DYNAMIC_DAH_TO_DIT_RATIO_UPPER_LIMIT_WPM, DYNAMIC_DAH_TO_DIT_RATIO_LOWER_LIMIT_RATIO, DYNAMIC_DAH_TO_DIT_RATIO_UPPER_LIMIT_RATIO);
            configuration.dah_to_dit_ratio = dynamicweightvalue;
        }
#endif //FEATURE_DYNAMIC_DAH_TO_DIT_RATIO

#ifdef FEATURE_LED_RING
        update_led_ring();
#endif //FEATURE_LED_RING

#ifdef FEATURE_DISPLAY
        lcd_center_print_timed_wpm();
#endif
    }
}
//-------------------------------------------------------------------------------------------------------

void command_speed_set(int wpm_set)
{
    if ((wpm_set > 0) && (wpm_set < 1000))
    {
        configuration.wpm_command_mode = wpm_set;
        config_dirty = 1;

#ifdef FEATURE_DISPLAY
        lcd_center_print_timed("Cmd Spd " + String(configuration.wpm_command_mode) + " wpm", 0, default_display_msg_delay);
#endif // FEATURE_DISPLAY
    }  // end if
} // end command_speed_set

#ifdef FEATURE_POTENTIOMETER
void check_potentiometer()
{

    static unsigned long last_pot_check_time = 0;

    if ((configuration.pot_activated || potentiometer_always_on) && ((millis() - last_pot_check_time) > potentiometer_check_interval_ms))
    {
        last_pot_check_time = millis();
        if ((potentiometer_enable_pin) && (digitalRead(potentiometer_enable_pin) == HIGH))
        {
            return;
        }
        byte pot_value_wpm_read = pot_value_wpm();
#ifdef ESP32
        if (((abs(pot_value_wpm_read - last_pot_wpm_read) * 10) > (potentiometer_change_threshold * 10)))
        {
#elif
        if (((abs(pot_value_wpm_read - last_pot_wpm_read) * 10) > (potentiometer_change_threshold * 10)))
        {
#endif
            //#ifdef DEBUG_POTENTIOMETER
            debug_serial_port->print(F("check_potentiometer: speed change: "));
            debug_serial_port->print(pot_value_wpm_read);
            debug_serial_port->print(F(" analog read: "));
            debug_serial_port->println(analogRead(potentiometer));
            //#endif
            if (keyer_machine_mode == KEYER_COMMAND_MODE)
                command_speed_set(pot_value_wpm_read);
            else
                speed_set(pot_value_wpm_read);
            last_pot_wpm_read = pot_value_wpm_read;
        }
    }
}

#endif
//-------------------------------------------------------------------------------------------------------
