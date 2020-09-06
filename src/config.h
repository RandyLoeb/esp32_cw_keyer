#ifndef CONFIG_H
#define CONFIG_H

byte sending_mode = UNDEFINED_SENDING;
byte command_mode_disable_tx = 0;

#if defined M5CORE
#define tx_key_line_1 0
#endif

byte current_tx_key_line = tx_key_line_1;

byte manual_ptt_invoke = 0;
byte qrss_dit_length = initial_qrss_dit_length;
byte keyer_machine_mode = KEYER_NORMAL; // KEYER_NORMAL, BEACON, KEYER_COMMAND_MODE
byte char_send_mode = 0;                // CW, HELL, AMERICAN_MORSE
byte key_tx = 0;                        // 0 = tx_key_line control suppressed
byte dit_buffer = 0;                    // used for buffering paddle hits in iambic operation
byte dah_buffer = 0;                    // used for buffering paddle hits in iambic operation
byte button0_buffer = 0;
byte being_sent = 0; // SENDING_NOTHING, SENDING_DIT, SENDING_DAH
byte key_state = 0;  // 0 = key up, 1 = key down
//byte config_dirty = 0;
unsigned long ptt_time = 0;
byte ptt_line_activated = 0;
byte speed_mode = SPEED_NORMAL;
#if defined(FEATURE_COMMAND_LINE_INTERFACE) || defined(FEATURE_PS2_KEYBOARD) || defined(FEATURE_MEMORY_MACROS) || defined(FEATURE_MEMORIES) || defined(FEATURE_COMMAND_BUTTONS)
unsigned int serial_number = 1;
#endif
byte pause_sending_buffer = 0;
byte length_letterspace = default_length_letterspace;
byte keying_compensation = default_keying_compensation;
byte first_extension_time = default_first_extension_time;
byte ultimatic_mode = ULTIMATIC_NORMAL;
float ptt_hang_time_wordspace_units = default_ptt_hang_time_wordspace_units;
byte last_sending_mode = MANUAL_SENDING;
byte zero = 0;
byte iambic_flag = 0;
unsigned long last_config_write = 0;

#if defined(FEATURE_SERIAL)
#if !defined(OPTION_DISABLE_SERIAL_PORT_CHECKING_WHILE_SENDING_CW)
byte loop_element_lengths_breakout_flag;
byte dump_current_character_flag;
#endif
byte incoming_serial_byte;
long primary_serial_port_baud_rate;
byte cw_send_echo_inhibit = 0;
#ifdef FEATURE_COMMAND_LINE_INTERFACE
byte serial_backslash_command;
byte cli_paddle_echo = cli_paddle_echo_on_at_boot;
byte cli_prosign_flag = 0;
byte cli_wait_for_cr_to_send_cw = 0;
#if defined(FEATURE_STRAIGHT_KEY_ECHO)
byte cli_straight_key_echo = cli_straight_key_echo_on_at_boot;
#endif
#endif //FEATURE_COMMAND_LINE_INTERFACE
#endif //FEATURE_SERIAL

byte send_buffer_array[send_buffer_size];
byte send_buffer_bytes = 0;
byte send_buffer_status = SERIAL_SEND_BUFFER_NORMAL;

#if defined(FEATURE_SERIAL)
byte primary_serial_port_mode = SERIAL_CLI;
#endif //FEATURE_SERIAL

PRIMARY_SERIAL_CLS *primary_serial_port;

PRIMARY_SERIAL_CLS *debug_serial_port;

#if defined(FEATURE_PADDLE_ECHO)
byte paddle_echo = 0;
long paddle_echo_buffer = 0;
unsigned long paddle_echo_buffer_decode_time = 0;
#endif //FEATURE_PADDLE_ECHO

unsigned long automatic_sending_interruption_time = 0;

byte async_eeprom_write = 0;
unsigned long millis_rollover = 0;
#endif
