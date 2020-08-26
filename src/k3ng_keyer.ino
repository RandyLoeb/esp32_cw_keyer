#define CODE_VERSION "2020.02.13.01"
#define eeprom_magic_number 35 // you can change this number to have the unit re-initialize EEPROM

#include <stdio.h>
#include "keyer_hardware.h"
#include "keyer_features_and_options.h"
#include "keyer.h"
#include "keyer_dependencies.h"
#include "keyer_debug.h"
#include "keyer_pin_settings.h"
#include "keyer_settings.h"
//#include "display.h"
#include "config.h"
#include "potentiometer.h"

#define memory_area_start 113

// Variables and stuff

#if defined(ESP32)
void add_to_send_buffer(byte incoming_serial_byte);
#include "keyer_esp32now.h"
#include "keyer_esp32.h"

#endif

void setup()
{
#ifdef ESPNOW_WIRELESS_KEYER
  initialize_espnow_wireless(speed_set);
#endif
  initialize_pins();
  initialize_serial_ports();
  initializeSpiffs(primary_serial_port);
  initDisplay();
  // Goody - this is available for testing startup issues
  initialize_keyer_state();
  initialize_potentiometer();

  initialize_default_modes();

  check_eeprom_for_initialization();
  check_for_beacon_mode();
  initialize_display();
}

// --------------------------------------------------------------------------------------------

void loop()
{

  if (keyer_machine_mode == KEYER_NORMAL)
  {

    check_paddles();
    service_dit_dah_buffers();

#if defined(FEATURE_SERIAL)
    check_serial();
    check_paddles();
    service_dit_dah_buffers();
#endif //FEATURE_SERIAL

    service_send_buffer(PRINTCHAR);
    check_ptt_tail();

#ifdef FEATURE_POTENTIOMETER
    check_potentiometer();
#endif //FEATURE_POTENTIOMETER

    check_for_dirty_configuration();

#ifdef FEATURE_DISPLAY
    check_paddles();
    service_send_buffer(PRINTCHAR);
    service_display();
#endif //FEATURE_DISPLAY

#ifdef FEATURE_PADDLE_ECHO
    service_paddle_echo();
#endif

    service_async_eeprom_write();
  }

  service_millis_rollover();
}

byte service_tx_inhibit_and_pause()
{

  byte return_code = 0;
  static byte pause_sending_buffer_active = 0;

  if (tx_inhibit_pin)
  {
    if ((digitalRead(tx_inhibit_pin) == tx_inhibit_pin_active_state))
    {
      dit_buffer = 0;
      dah_buffer = 0;
      return_code = 1;
      if (send_buffer_bytes)
      {
        clear_send_buffer();
        send_buffer_status = SERIAL_SEND_BUFFER_NORMAL;
      }
    }
  }

  if (tx_pause_pin)
  {
    if ((digitalRead(tx_pause_pin) == tx_pause_pin_active_state))
    {
      dit_buffer = 0;
      dah_buffer = 0;
      return_code = 1;
      if (!pause_sending_buffer_active)
      {
        pause_sending_buffer = 1;
        pause_sending_buffer_active = 1;
        delay(10);
      }
    }
    else
    {
      if (pause_sending_buffer_active)
      {
        pause_sending_buffer = 0;
        pause_sending_buffer_active = 0;
        delay(10);
      }
    }
  }

  return return_code;
}

void check_for_dirty_configuration()
{

  if (config_dirty)
  {
    write_settings_to_eeprom(0);
  }
}

//-------------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------------

void check_paddles()
{

#ifdef DEBUG_LOOP
  debug_serial_port->println(F("loop: entering check_paddles"));
#endif

#define NO_CLOSURE 0
#define DIT_CLOSURE_DAH_OFF 1
#define DAH_CLOSURE_DIT_OFF 2
#define DIT_CLOSURE_DAH_ON 3
#define DAH_CLOSURE_DIT_ON 4

  if (keyer_machine_mode == BEACON)
  {
    return;
  }

  static byte last_closure = NO_CLOSURE;

  check_dit_paddle();
  check_dah_paddle();

#ifdef FEATURE_WINKEY_EMULATION
  if (winkey_dit_invoke)
  {
    dit_buffer = 1;
  }
  if (winkey_dah_invoke)
  {
    dah_buffer = 1;
  }
#endif //FEATURE_WINKEY_EMULATION

  if (configuration.keyer_mode == ULTIMATIC)
  {
    if (ultimatic_mode == ULTIMATIC_NORMAL)
    {

      switch (last_closure)
      {
      case DIT_CLOSURE_DAH_OFF:
        if (dah_buffer)
        {
          if (dit_buffer)
          {
            last_closure = DAH_CLOSURE_DIT_ON;
            dit_buffer = 0;
          }
          else
          {
            last_closure = DAH_CLOSURE_DIT_OFF;
          }
        }
        else
        {
          if (!dit_buffer)
          {
            last_closure = NO_CLOSURE;
          }
        }
        break;
      case DIT_CLOSURE_DAH_ON:
        if (dit_buffer)
        {
          if (dah_buffer)
          {
            dah_buffer = 0;
          }
          else
          {
            last_closure = DIT_CLOSURE_DAH_OFF;
          }
        }
        else
        {
          if (dah_buffer)
          {
            last_closure = DAH_CLOSURE_DIT_OFF;
          }
          else
          {
            last_closure = NO_CLOSURE;
          }
        }
        break;

      case DAH_CLOSURE_DIT_OFF:
        if (dit_buffer)
        {
          if (dah_buffer)
          {
            last_closure = DIT_CLOSURE_DAH_ON;
            dah_buffer = 0;
          }
          else
          {
            last_closure = DIT_CLOSURE_DAH_OFF;
          }
        }
        else
        {
          if (!dah_buffer)
          {
            last_closure = NO_CLOSURE;
          }
        }
        break;

      case DAH_CLOSURE_DIT_ON:
        if (dah_buffer)
        {
          if (dit_buffer)
          {
            dit_buffer = 0;
          }
          else
          {
            last_closure = DAH_CLOSURE_DIT_OFF;
          }
        }
        else
        {
          if (dit_buffer)
          {
            last_closure = DIT_CLOSURE_DAH_OFF;
          }
          else
          {
            last_closure = NO_CLOSURE;
          }
        }
        break;

      case NO_CLOSURE:
        if ((dit_buffer) && (!dah_buffer))
        {
          last_closure = DIT_CLOSURE_DAH_OFF;
        }
        else
        {
          if ((dah_buffer) && (!dit_buffer))
          {
            last_closure = DAH_CLOSURE_DIT_OFF;
          }
          else
          {
            if ((dit_buffer) && (dah_buffer))
            {
              // need to handle dit/dah priority here
              last_closure = DIT_CLOSURE_DAH_ON;
              dah_buffer = 0;
            }
          }
        }
        break;
      }
    }
    else
    { // if (ultimatic_mode == ULTIMATIC_NORMAL)
      if ((dit_buffer) && (dah_buffer))
      { // dit or dah priority mode
        if (ultimatic_mode == ULTIMATIC_DIT_PRIORITY)
        {
          dah_buffer = 0;
        }
        else
        {
          dit_buffer = 0;
        }
      }
    } // if (ultimatic_mode == ULTIMATIC_NORMAL)
  }   // if (configuration.keyer_mode == ULTIMATIC)

  if (configuration.keyer_mode == SINGLE_PADDLE)
  {
    switch (last_closure)
    {
    case DIT_CLOSURE_DAH_OFF:
      if (dit_buffer)
      {
        if (dah_buffer)
        {
          dah_buffer = 0;
        }
        else
        {
          last_closure = DIT_CLOSURE_DAH_OFF;
        }
      }
      else
      {
        if (dah_buffer)
        {
          last_closure = DAH_CLOSURE_DIT_OFF;
        }
        else
        {
          last_closure = NO_CLOSURE;
        }
      }
      break;

    case DIT_CLOSURE_DAH_ON:

      if (dah_buffer)
      {
        if (dit_buffer)
        {
          last_closure = DAH_CLOSURE_DIT_ON;
          dit_buffer = 0;
        }
        else
        {
          last_closure = DAH_CLOSURE_DIT_OFF;
        }
      }
      else
      {
        if (!dit_buffer)
        {
          last_closure = NO_CLOSURE;
        }
      }
      break;

    case DAH_CLOSURE_DIT_OFF:
      if (dah_buffer)
      {
        if (dit_buffer)
        {
          dit_buffer = 0;
        }
        else
        {
          last_closure = DAH_CLOSURE_DIT_OFF;
        }
      }
      else
      {
        if (dit_buffer)
        {
          last_closure = DIT_CLOSURE_DAH_OFF;
        }
        else
        {
          last_closure = NO_CLOSURE;
        }
      }
      break;

    case DAH_CLOSURE_DIT_ON:
      if (dit_buffer)
      {
        if (dah_buffer)
        {
          last_closure = DIT_CLOSURE_DAH_ON;
          dah_buffer = 0;
        }
        else
        {
          last_closure = DIT_CLOSURE_DAH_OFF;
        }
      }
      else
      {
        if (!dah_buffer)
        {
          last_closure = NO_CLOSURE;
        }
      }
      break;

    case NO_CLOSURE:
      if ((dit_buffer) && (!dah_buffer))
      {
        last_closure = DIT_CLOSURE_DAH_OFF;
      }
      else
      {
        if ((dah_buffer) && (!dit_buffer))
        {
          last_closure = DAH_CLOSURE_DIT_OFF;
        }
        else
        {
          if ((dit_buffer) && (dah_buffer))
          {
            // need to handle dit/dah priority here
            last_closure = DIT_CLOSURE_DAH_ON;
            dah_buffer = 0;
          }
        }
      }
      break;
    }
  } //if (configuration.keyer_mode == SINGLE_PADDLE)

  service_tx_inhibit_and_pause();
}

//-------------------------------------------------------------------------------------------------------

void ptt_key()
{

  unsigned long ptt_activation_time = millis();
  byte all_delays_satisfied = 0;

#ifdef FEATURE_SEQUENCER
  byte sequencer_1_ok = 0;
  byte sequencer_2_ok = 0;
  byte sequencer_3_ok = 0;
  byte sequencer_4_ok = 0;
  byte sequencer_5_ok = 0;
#endif

  if (ptt_line_activated == 0)
  { // if PTT is currently deactivated, bring it up and insert PTT lead time delay

    if (configuration.current_ptt_line)
    {

      digitalWrite(configuration.current_ptt_line, ptt_line_active_state);
    }

    ptt_line_activated = 1;

    while (!all_delays_satisfied)
    {

      if ((millis() - ptt_activation_time) >= configuration.ptt_lead_time[configuration.current_tx - 1])
      {
        all_delays_satisfied = 1;
      }

    } //while (!all_delays_satisfied)
  }
  ptt_time = millis();
}
//-------------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------------
void ptt_unkey()
{

  if (ptt_line_activated)
  {

#ifdef FEATURE_SO2R_BASE
    if (current_tx_ptt_line)
    {
      digitalWrite(current_tx_ptt_line, ptt_line_inactive_state);
#else
    if (configuration.current_ptt_line)
    {
      digitalWrite(configuration.current_ptt_line, ptt_line_inactive_state);

#endif //FEATURE_SO2R_BASE
    }
    ptt_line_activated = 0;
  }
}

//-------------------------------------------------------------------------------------------------------
void check_ptt_tail()
{

  static byte manual_ptt_invoke_ptt_input_pin = 0;

  if (ptt_input_pin)
  {
    if ((digitalRead(ptt_input_pin) == ptt_input_pin_active_state))
    {
      if (!manual_ptt_invoke)
      {
        manual_ptt_invoke = 1;
        manual_ptt_invoke_ptt_input_pin = 1;
        ptt_key();
        return;
      }
    }
    else
    {
      if ((manual_ptt_invoke) && (manual_ptt_invoke_ptt_input_pin))
      {
        manual_ptt_invoke = 0;
        manual_ptt_invoke_ptt_input_pin = 0;
        if (!key_state)
        {
          ptt_unkey();
        }
      }
    }
  }

#if !defined(FEATURE_WINKEY_EMULATION)
  if (key_state)
  {
    ptt_time = millis();
  }
  else
  {
    if ((ptt_line_activated) && (manual_ptt_invoke == 0))
    {
      //if ((millis() - ptt_time) > ptt_tail_time) {
      if (last_sending_mode == MANUAL_SENDING)
      {
#ifndef OPTION_INCLUDE_PTT_TAIL_FOR_MANUAL_SENDING

        // PTT Tail Time: N     PTT Hang Time: Y

        if ((millis() - ptt_time) >= ((configuration.length_wordspace * ptt_hang_time_wordspace_units) * float(1200 / configuration.wpm)))
        {
          ptt_unkey();
        }
#else //ndef OPTION_INCLUDE_PTT_TAIL_FOR_MANUAL_SENDING
#ifndef OPTION_EXCLUDE_PTT_HANG_TIME_FOR_MANUAL_SENDING

        // PTT Tail Time: Y     PTT Hang Time: Y

        if ((millis() - ptt_time) >= (((configuration.length_wordspace * ptt_hang_time_wordspace_units) * float(1200 / configuration.wpm)) + configuration.ptt_tail_time[configuration.current_tx - 1]))
        {
          ptt_unkey();
        }
#else  //OPTION_EXCLUDE_PTT_HANG_TIME_FOR_MANUAL_SENDING
        if ((millis() - ptt_time) >= configuration.ptt_tail_time[configuration.current_tx - 1])
        {

          // PTT Tail Time: Y    PTT Hang Time: N

          ptt_unkey();
        }
#endif //OPTION_EXCLUDE_PTT_HANG_TIME_FOR_MANUAL_SENDING
#endif //ndef OPTION_INCLUDE_PTT_TAIL_FOR_MANUAL_SENDING
      }
      else
      { // automatic sending
        if (((millis() - ptt_time) > configuration.ptt_tail_time[configuration.current_tx - 1]) && (!configuration.ptt_buffer_hold_active || ((!send_buffer_bytes) && configuration.ptt_buffer_hold_active) || (pause_sending_buffer)))
        {
          ptt_unkey();
        }
      }
    }
  }

#endif //FEATURE_WINKEY_EMULATION
}

//-------------------------------------------------------------------------------------------------------
void write_settings_to_eeprom(int initialize_eeprom)
{

  writeConfigurationToFile();

  config_dirty = 0;
}

//-------------------------------------------------------------------------------------------------------

void service_async_eeprom_write()
{

  // This writes one byte out to EEPROM each time it is called

  static byte last_async_eeprom_write_status = 0;
  static int ee = 0;
  static unsigned int i = 0;
  static const byte *p;

  if ((async_eeprom_write) && (!send_buffer_bytes) && (!ptt_line_activated) && (!dit_buffer) && (!dah_buffer) && (paddle_pin_read(paddle_left) == HIGH) && (paddle_pin_read(paddle_right) == HIGH))
  {
    if (last_async_eeprom_write_status)
    { // we have an ansynchronous write to eeprom in progress

#if defined(_BOARD_PIC32_PINGUINO_)
      if (EEPROM.read(ee) != *p)
      {
        EEPROM.write(ee, *p);
      }
      ee++;
      p++;
#else
#if !defined(ESP32)
      EEPROM.update(ee++, *p++);
#endif
#endif

      if (i < sizeof(configuration))
      {
#if defined(DEBUG_ASYNC_EEPROM_WRITE)
        debug_serial_port->print(F("service_async_eeprom_write: write: "));
        debug_serial_port->println(i);
#endif
        i++;
      }
      else
      { // we're done
        async_eeprom_write = 0;
        last_async_eeprom_write_status = 0;
#if defined(DEBUG_ASYNC_EEPROM_WRITE)
        debug_serial_port->println(F("service_async_eeprom_write: complete"));
#endif
      }
    }
    else
    { // we don't have one in progress - initialize things

      p = (const byte *)(const void *)&configuration;
      ee = 1; // starting point of configuration struct
      i = 0;
      last_async_eeprom_write_status = 1;
#if defined(DEBUG_ASYNC_EEPROM_WRITE)
      debug_serial_port->println(F("service_async_eeprom_write: init"));
#endif
    }
  }
}

//-------------------------------------------------------------------------------------------------------

int read_settings_from_eeprom()
{

  // returns 0 if eeprom had valid settings, returns 1 if eeprom needs initialized

  if (configFileExists())
  {
    setConfigurationFromFile();
    //all good, return 0
    return 0;
  }
  else
  {
    //no file so initialize it
    return 1;
  }

  return 1;
}

//-------------------------------------------------------------------------------------------------------

void check_dit_paddle()
{

  byte pin_value = 0;
  byte dit_paddle = 0;
#ifdef OPTION_DIT_PADDLE_NO_SEND_ON_MEM_RPT
  static byte memory_rpt_interrupt_flag = 0;
#endif

  if (configuration.paddle_mode == PADDLE_NORMAL)
  {
    dit_paddle = paddle_left;
  }
  else
  {
    dit_paddle = paddle_right;
  }

  pin_value = getEspNowBuff(ESPNOW_DIT) && paddle_pin_read(dit_paddle);

#if defined(FEATURE_USB_MOUSE) || defined(FEATURE_USB_KEYBOARD)
  if (usb_dit)
  {
    pin_value = 0;
  }
#endif

#ifdef OPTION_DIT_PADDLE_NO_SEND_ON_MEM_RPT
  if (pin_value && memory_rpt_interrupt_flag)
  {
    memory_rpt_interrupt_flag = 0;
    sending_mode = MANUAL_SENDING;
    loop_element_lengths(3, 0, configuration.wpm);
    dit_buffer = 0;
  }
#endif

#ifdef OPTION_DIT_PADDLE_NO_SEND_ON_MEM_RPT
  if ((pin_value == 0) && (memory_rpt_interrupt_flag == 0))
  {
#else
  if (pin_value == 0)
  {
#endif
#ifdef FEATURE_DEAD_OP_WATCHDOG
    if (dit_buffer == 0)
    {
      dit_counter++;
      dah_counter = 0;
    }
#endif
    dit_buffer = 1;

#if defined(OPTION_WINKEY_SEND_BREAKIN_STATUS_BYTE) && defined(FEATURE_WINKEY_EMULATION)
    if (!winkey_interrupted && winkey_host_open && !winkey_breakin_status_byte_inhibit)
    {
      send_winkey_breakin_byte_flag = 1;
      // winkey_port_write(0xc2|winkey_sending|winkey_xoff); // 0xc2 - BREAKIN bit set high
      // winkey_interrupted = 1;

      // tone(sidetone_line,1000);
      // delay(500);
      // noTone(sidetone_line);

      dit_buffer = 0;
    }
#endif //defined(OPTION_WINKEY_SEND_BREAKIN_STATUS_BYTE) && defined(FEATURE_WINKEY_EMULATION)

#ifdef FEATURE_SLEEP
    last_activity_time = millis();
#endif //FEATURE_SLEEP
    manual_ptt_invoke = 0;
#ifdef FEATURE_MEMORIES
    if (repeat_memory < 255)
    {
      repeat_memory = 255;
      clear_send_buffer();
#ifdef OPTION_DIT_PADDLE_NO_SEND_ON_MEM_RPT
      dit_buffer = 0;
      while (!paddle_pin_read(dit_paddle))
      {
      };
      memory_rpt_interrupt_flag = 1;
#endif
    }
#endif
  }
}

//-------------------------------------------------------------------------------------------------------

void check_dah_paddle()
{

  byte pin_value = 0;
  byte dah_paddle;

  if (configuration.paddle_mode == PADDLE_NORMAL)
  {
    dah_paddle = paddle_right;
  }
  else
  {
    dah_paddle = paddle_left;
  }

  pin_value = getEspNowBuff(ESPNOW_DAH) && paddle_pin_read(dah_paddle);

#if defined(FEATURE_USB_MOUSE) || defined(FEATURE_USB_KEYBOARD)
  if (usb_dah)
  {
    pin_value = 0;
  }
#endif

  if (pin_value == 0)
  {
#ifdef FEATURE_DEAD_OP_WATCHDOG
    if (dah_buffer == 0)
    {
      dah_counter++;
      dit_counter = 0;
    }
#endif
    dah_buffer = 1;

#if defined(OPTION_WINKEY_SEND_BREAKIN_STATUS_BYTE) && defined(FEATURE_WINKEY_EMULATION)
    if (!winkey_interrupted && winkey_host_open && !winkey_breakin_status_byte_inhibit)
    {
      send_winkey_breakin_byte_flag = 1;
      dah_buffer = 0;
    }
#endif //defined(OPTION_WINKEY_SEND_BREAKIN_STATUS_BYTE) && defined(FEATURE_WINKEY_EMULATION)

#ifdef FEATURE_SLEEP
    last_activity_time = millis();
#endif //FEATURE_SLEEP
#ifdef FEATURE_MEMORIES
    repeat_memory = 255;
#endif
    manual_ptt_invoke = 0;
  }
}

//-------------------------------------------------------------------------------------------------------

void send_dit()
{
  sendEspNowDitDah(ESPNOW_DIT);
  // notes: key_compensation is a straight x mS lengthening or shortening of the key down time
  //        weighting is

  unsigned int character_wpm = configuration.wpm;

#ifdef FEATURE_FARNSWORTH
  if ((sending_mode == AUTOMATIC_SENDING) && (configuration.wpm_farnsworth > configuration.wpm))
  {
    character_wpm = configuration.wpm_farnsworth;
#if defined(DEBUG_FARNSWORTH)
    debug_serial_port->println(F("send_dit: farns act"));
#endif
  }
#if defined(DEBUG_FARNSWORTH)

  else
  {
    debug_serial_port->println(F("send_dit: farns inact"));
  }
#endif
#endif //FEATURE_FARNSWORTH

  if (keyer_machine_mode == KEYER_COMMAND_MODE)
  {
    character_wpm = configuration.wpm_command_mode;
  }

  being_sent = SENDING_DIT;
  tx_and_sidetone_key(1);
#ifdef DEBUG_VARIABLE_DUMP
  dit_start_time = millis();
#endif
  if ((tx_key_dit) && (key_tx))
  {
    digitalWrite(tx_key_dit, tx_key_dit_and_dah_pins_active_state);
  }

#ifdef FEATURE_QLF
  if (qlf_active)
  {
    loop_element_lengths((1.0 * (float(configuration.weighting) / 50) * (random(qlf_dit_min, qlf_dit_max) / 100.0)), keying_compensation, character_wpm);
  }
  else
  {
    loop_element_lengths((1.0 * (float(configuration.weighting) / 50)), keying_compensation, character_wpm);
  }
#else  //FEATURE_QLF
  loop_element_lengths((1.0 * (float(configuration.weighting) / 50)), keying_compensation, character_wpm);
#endif //FEATURE_QLF

  if ((tx_key_dit) && (key_tx))
  {
    digitalWrite(tx_key_dit, tx_key_dit_and_dah_pins_inactive_state);
  }
#ifdef DEBUG_VARIABLE_DUMP
  dit_end_time = millis();
#endif
  tx_and_sidetone_key(0);

  loop_element_lengths((2.0 - (float(configuration.weighting) / 50)), (-1.0 * keying_compensation), character_wpm);

#ifdef FEATURE_AUTOSPACE

  byte autospace_end_of_character_flag = 0;

  if ((sending_mode == MANUAL_SENDING) && (configuration.autospace_active))
  {
    check_paddles();
  }
  if ((sending_mode == MANUAL_SENDING) && (configuration.autospace_active) && (dit_buffer == 0) && (dah_buffer == 0))
  {
    loop_element_lengths(2, 0, configuration.wpm);
    autospace_end_of_character_flag = 1;
  }
#endif

#ifdef FEATURE_WINKEY_EMULATION
  if ((winkey_host_open) && (winkey_paddle_echo_activated) && (sending_mode == MANUAL_SENDING))
  {
    winkey_paddle_echo_buffer = (winkey_paddle_echo_buffer * 10) + 1;
    //winkey_paddle_echo_buffer_decode_time = millis() + (float(winkey_paddle_echo_buffer_decode_time_factor/float(configuration.wpm))*length_letterspace);
    winkey_paddle_echo_buffer_decode_time = millis() + (float(winkey_paddle_echo_buffer_decode_timing_factor * 1200.0 / float(configuration.wpm)) * length_letterspace);

#ifdef FEATURE_AUTOSPACE
    if (autospace_end_of_character_flag)
    {
      winkey_paddle_echo_buffer_decode_time = 0;
    }
#endif //FEATURE_AUTOSPACE
  }
#endif

#ifdef FEATURE_PADDLE_ECHO
  if (sending_mode == MANUAL_SENDING)
  {
    paddle_echo_buffer = (paddle_echo_buffer * 10) + 1;
    paddle_echo_buffer_decode_time = millis() + (float((cw_echo_timing_factor * 1200.0) / configuration.wpm) * length_letterspace);

#ifdef FEATURE_AUTOSPACE
    if (autospace_end_of_character_flag)
    {
      paddle_echo_buffer_decode_time = 0;
    }
#endif //FEATURE_AUTOSPACE
  }
#endif //FEATURE_PADDLE_ECHO

#ifdef FEATURE_AUTOSPACE
  autospace_end_of_character_flag = 0;
#endif //FEATURE_AUTOSPACE

  being_sent = SENDING_NOTHING;
  last_sending_mode = sending_mode;

  check_paddles();
}

//-------------------------------------------------------------------------------------------------------

void send_dah()
{
  sendEspNowDitDah(ESPNOW_DAH);
  unsigned int character_wpm = configuration.wpm;

#ifdef FEATURE_FARNSWORTH
  if ((sending_mode == AUTOMATIC_SENDING) && (configuration.wpm_farnsworth > configuration.wpm))
  {
    character_wpm = configuration.wpm_farnsworth;
  }
#endif //FEATURE_FARNSWORTH

  if (keyer_machine_mode == KEYER_COMMAND_MODE)
  {
    character_wpm = configuration.wpm_command_mode;
  }

  being_sent = SENDING_DAH;
  tx_and_sidetone_key(1);
#ifdef DEBUG_VARIABLE_DUMP
  dah_start_time = millis();
#endif
  if ((tx_key_dah) && (key_tx))
  {
    digitalWrite(tx_key_dah, tx_key_dit_and_dah_pins_active_state);
  }

#ifdef FEATURE_QLF
  if (qlf_active)
  {
    loop_element_lengths((float(configuration.dah_to_dit_ratio / 100.0) * (float(configuration.weighting) / 50) * (random(qlf_dah_min, qlf_dah_max) / 100.0)), keying_compensation, character_wpm);
  }
  else
  {
    loop_element_lengths((float(configuration.dah_to_dit_ratio / 100.0) * (float(configuration.weighting) / 50)), keying_compensation, character_wpm);
  }
#else  //FEATURE_QLF
  loop_element_lengths((float(configuration.dah_to_dit_ratio / 100.0) * (float(configuration.weighting) / 50)), keying_compensation, character_wpm);
#endif //FEATURE_QLF

  if ((tx_key_dah) && (key_tx))
  {
    digitalWrite(tx_key_dah, tx_key_dit_and_dah_pins_inactive_state);
  }

#ifdef DEBUG_VARIABLE_DUMP
  dah_end_time = millis();
#endif

  tx_and_sidetone_key(0);

  loop_element_lengths((4.0 - (3.0 * (float(configuration.weighting) / 50))), (-1.0 * keying_compensation), character_wpm);

#ifdef FEATURE_AUTOSPACE

  byte autospace_end_of_character_flag = 0;

  if ((sending_mode == MANUAL_SENDING) && (configuration.autospace_active))
  {
    check_paddles();
  }
  if ((sending_mode == MANUAL_SENDING) && (configuration.autospace_active) && (dit_buffer == 0) && (dah_buffer == 0))
  {
    loop_element_lengths(2, 0, configuration.wpm);
    autospace_end_of_character_flag = 1;
  }
#endif

#ifdef FEATURE_WINKEY_EMULATION
  if ((winkey_host_open) && (winkey_paddle_echo_activated) && (sending_mode == MANUAL_SENDING))
  {
    winkey_paddle_echo_buffer = (winkey_paddle_echo_buffer * 10) + 2;
    //winkey_paddle_echo_buffer_decode_time = millis() + (float(winkey_paddle_echo_buffer_decode_time_factor/float(configuration.wpm))*length_letterspace);
    winkey_paddle_echo_buffer_decode_time = millis() + (float(winkey_paddle_echo_buffer_decode_timing_factor * 1200.0 / float(configuration.wpm)) * length_letterspace);
#ifdef FEATURE_AUTOSPACE
    if (autospace_end_of_character_flag)
    {
      winkey_paddle_echo_buffer_decode_time = 0;
    }
#endif //FEATURE_AUTOSPACE
  }
#endif

#ifdef FEATURE_PADDLE_ECHO
  if (sending_mode == MANUAL_SENDING)
  {
    paddle_echo_buffer = (paddle_echo_buffer * 10) + 2;
    paddle_echo_buffer_decode_time = millis() + (float((cw_echo_timing_factor * 1200.0) / configuration.wpm) * length_letterspace);

#ifdef FEATURE_AUTOSPACE
    if (autospace_end_of_character_flag)
    {
      paddle_echo_buffer_decode_time = 0;
    }
#endif //FEATURE_AUTOSPACE
  }
#endif //FEATURE_PADDLE_ECHO

#ifdef FEATURE_AUTOSPACE
  autospace_end_of_character_flag = 0;
#endif //FEATURE_AUTOSPACE

  check_paddles();

  being_sent = SENDING_NOTHING;
  last_sending_mode = sending_mode;
}

//-------------------------------------------------------------------------------------------------------

void tx_and_sidetone_key(int state)
{

#if !defined(FEATURE_PTT_INTERLOCK)
  if ((state) && (key_state == 0))
  {
    if (key_tx)
    {
      byte previous_ptt_line_activated = ptt_line_activated;
      ptt_key();
      if (current_tx_key_line)
      {
        digitalWrite(current_tx_key_line, tx_key_line_active_state);
      }
#if defined(OPTION_WINKEY_2_SUPPORT) && defined(FEATURE_WINKEY_EMULATION) && !defined(FEATURE_SO2R_BASE)
      if ((wk2_both_tx_activated) && (tx_key_line_2))
      {
        digitalWrite(tx_key_line_2, HIGH);
      }
#endif
      if ((first_extension_time) && (previous_ptt_line_activated == 0))
      {
        delay(first_extension_time);
      }
    }
    if ((configuration.sidetone_mode == SIDETONE_ON) || (keyer_machine_mode == KEYER_COMMAND_MODE) || ((configuration.sidetone_mode == SIDETONE_PADDLE_ONLY) && (sending_mode == MANUAL_SENDING)))
    {
#if !defined(OPTION_SIDETONE_DIGITAL_OUTPUT_NO_SQUARE_WAVE)
      tone(sidetone_line, configuration.hz_sidetone);
#else
      if (sidetone_line)
      {
        digitalWrite(sidetone_line, HIGH);
      }
#endif
    }
    key_state = 1;
  }
  else
  {
    if ((state == 0) && (key_state))
    {
      if (key_tx)
      {
        if (current_tx_key_line)
        {
          digitalWrite(current_tx_key_line, tx_key_line_inactive_state);
        }
#if defined(OPTION_WINKEY_2_SUPPORT) && defined(FEATURE_WINKEY_EMULATION) && !defined(FEATURE_SO2R_BASE)
        if ((wk2_both_tx_activated) && (tx_key_line_2))
        {
          digitalWrite(tx_key_line_2, LOW);
        }
#endif
        ptt_key();
      }
      if ((configuration.sidetone_mode == SIDETONE_ON) || (keyer_machine_mode == KEYER_COMMAND_MODE) || ((configuration.sidetone_mode == SIDETONE_PADDLE_ONLY) && (sending_mode == MANUAL_SENDING)))
      {
#if !defined(OPTION_SIDETONE_DIGITAL_OUTPUT_NO_SQUARE_WAVE)
        noTone(sidetone_line);
#else
        if (sidetone_line)
        {
          digitalWrite(sidetone_line, LOW);
        }
#endif
      }
      key_state = 0;
    }
  }

#endif //FEATURE_PTT_INTERLOCK

#if defined(FEATURE_INTERNET_LINK)
  link_key(state);
#endif

  check_ptt_tail();
}

//-------------------------------------------------------------------------------------------------------

void loop_element_lengths(float lengths, float additional_time_ms, int speed_wpm_in)
{

#if defined(FEATURE_SERIAL) && !defined(OPTION_DISABLE_SERIAL_PORT_CHECKING_WHILE_SENDING_CW)
  loop_element_lengths_breakout_flag = 1;
#endif //FEATURE_SERIAL

  float element_length;

  if (lengths <= 0)
  {
    return;
  }

#if defined(FEATURE_FARNSWORTH)

  if ((lengths == 1) && (speed_wpm_in == 0))
  {
    element_length = additional_time_ms;
  }
  else
  {
    if (speed_mode == SPEED_NORMAL)
    {
      element_length = 1200 / speed_wpm_in;
    }
    else
    {
      element_length = qrss_dit_length * 1000;
    }
  }

#else  //FEATURE_FARNSWORTH
  if (speed_mode == SPEED_NORMAL)
  {
    element_length = 1200 / speed_wpm_in;
  }
  else
  {
    element_length = qrss_dit_length * 1000;
  }
#endif //FEATURE_FARNSWORTH

  unsigned long ticks = long(element_length * lengths * 1000) + long(additional_time_ms * 1000); // improvement from Paul, K1XM
  unsigned long start = micros();

#if defined(FEATURE_SERIAL) && !defined(OPTION_DISABLE_SERIAL_PORT_CHECKING_WHILE_SENDING_CW)
  while (((micros() - start) < ticks) && (service_tx_inhibit_and_pause() == 0) && loop_element_lengths_breakout_flag)
  {
#else
  while (((micros() - start) < ticks) && (service_tx_inhibit_and_pause() == 0))
  {
#endif

    check_ptt_tail();

#if defined(FEATURE_SERIAL) && !defined(OPTION_DISABLE_SERIAL_PORT_CHECKING_WHILE_SENDING_CW)
    if (((ticks - (micros() - start)) > (10 * 1000)) && (sending_mode == AUTOMATIC_SENDING))
    {
      check_serial();
      if (loop_element_lengths_breakout_flag == 0)
      {
        dump_current_character_flag = 1;
      }
    }
#endif //FEATURE_SERIAL

#if defined(FEATURE_INTERNET_LINK) /*&& !defined(OPTION_INTERNET_LINK_NO_UDP_SVC_DURING_KEY_DOWN)*/
    //if ((millis() > 1000)  && ((millis()-start) > FEATURE_INTERNET_LINK_SVC_DURING_LOOP_TIME_MS)){
    if ((ticks - (micros() - start)) > (FEATURE_INTERNET_LINK_SVC_DURING_LOOP_TIME_MS * 1000))
    {
      service_udp_send_buffer();
      service_udp_receive();
      service_internet_link_udp_receive_buffer();
    }
#endif //FEATURE_INTERNET_LINK

#if defined(OPTION_WATCHDOG_TIMER)
    wdt_reset();
#endif //OPTION_WATCHDOG_TIMER

#if defined(FEATURE_ROTARY_ENCODER)
    check_rotary_encoder();
#endif //FEATURE_ROTARY_ENCODER

#if defined(FEATURE_USB_KEYBOARD) || defined(FEATURE_USB_MOUSE)
    service_usb();
#endif //FEATURE_USB_KEYBOARD || FEATURE_USB_MOUSE

#if defined(FEATURE_PTT_INTERLOCK)
    service_ptt_interlock();
#endif //FEATURE_PTT_INTERLOCK

#if defined(FEATURE_4x4_KEYPAD) || defined(FEATURE_3x4_KEYPAD)
    service_keypad();
#endif

#if defined(FEATURE_DISPLAY)
    if ((ticks - (micros() - start)) > (10 * 1000))
    {
      service_display();
    }
#endif

    if ((configuration.keyer_mode != ULTIMATIC) && (configuration.keyer_mode != SINGLE_PADDLE))
    {
      if ((configuration.keyer_mode == IAMBIC_A) && (paddle_pin_read(paddle_left) == LOW) && (paddle_pin_read(paddle_right) == LOW))
      {
        iambic_flag = 1;
      }

#ifndef FEATURE_CMOS_SUPER_KEYER_IAMBIC_B_TIMING
      if (being_sent == SENDING_DIT)
      {
        check_dah_paddle();
      }
      else
      {
        if (being_sent == SENDING_DAH)
        {
          check_dit_paddle();
        }
        else
        {
          check_dah_paddle();
          check_dit_paddle();
        }
      }
#else  ////FEATURE_CMOS_SUPER_KEYER_IAMBIC_B_TIMING
      if (configuration.cmos_super_keyer_iambic_b_timing_on)
      {
        if ((float(float(micros() - start) / float(ticks)) * 100) >= configuration.cmos_super_keyer_iambic_b_timing_percent)
        {
          //if ((float(float(millis()-starttime)/float(starttime-ticks))*100) >= configuration.cmos_super_keyer_iambic_b_timing_percent) {
          if (being_sent == SENDING_DIT)
          {
            check_dah_paddle();
          }
          else
          {
            if (being_sent == SENDING_DAH)
            {
              check_dit_paddle();
            }
          }
        }
        else
        {
          if (((being_sent == SENDING_DIT) || (being_sent == SENDING_DAH)) && (paddle_pin_read(paddle_left) == LOW) && (paddle_pin_read(paddle_right) == LOW))
          {
            dah_buffer = 0;
            dit_buffer = 0;
          }
        }
      }
      else
      {
        if (being_sent == SENDING_DIT)
        {
          check_dah_paddle();
        }
        else
        {
          if (being_sent == SENDING_DAH)
          {
            check_dit_paddle();
          }
          else
          {
            check_dah_paddle();
            check_dit_paddle();
          }
        }
      }
#endif //FEATURE_CMOS_SUPER_KEYER_IAMBIC_B_TIMING
    }
    else
    { //(configuration.keyer_mode != ULTIMATIC)

      if (being_sent == SENDING_DIT)
      {
        check_dah_paddle();
      }
      else
      {
        if (being_sent == SENDING_DAH)
        {
          check_dit_paddle();
        }
        else
        {
          check_dah_paddle();
          check_dit_paddle();
        }
      }
    }

#if defined(FEATURE_MEMORIES) && defined(FEATURE_COMMAND_BUTTONS)
    check_the_memory_buttons();
#endif

// blow out prematurely if we're automatic sending and a paddle gets hit
#ifdef FEATURE_COMMAND_BUTTONS
    if (sending_mode == AUTOMATIC_SENDING && (paddle_pin_read(paddle_left) == LOW || paddle_pin_read(paddle_right) == LOW || analogbuttonread(0) || dit_buffer || dah_buffer))
    {
      if (keyer_machine_mode == KEYER_NORMAL)
      {
        sending_mode = AUTOMATIC_SENDING_INTERRUPTED;
        automatic_sending_interruption_time = millis();
        return;
      }
    }
#else
    if (sending_mode == AUTOMATIC_SENDING && (paddle_pin_read(paddle_left) == LOW || paddle_pin_read(paddle_right) == LOW || dit_buffer || dah_buffer))
    {
      if (keyer_machine_mode == KEYER_NORMAL)
      {
        sending_mode = AUTOMATIC_SENDING_INTERRUPTED;
        automatic_sending_interruption_time = millis();

#ifdef FEATURE_SO2R_BASE
        so2r_set_rx();
#endif

        return;
      }
    }
#endif

#ifdef FEATURE_STRAIGHT_KEY
    service_straight_key();
#endif //FEATURE_STRAIGHT_KEY

#if defined(FEATURE_WEB_SERVER)
    if (speed_mode == SPEED_QRSS)
    {
      service_web_server();
    }
#endif //FEATURE_WEB_SERVER

#ifdef FEATURE_SO2R_SWITCHES
    so2r_switches();
#endif

  } //while ((millis() < endtime) && (millis() > 200))

  if ((configuration.keyer_mode == IAMBIC_A) && (iambic_flag) && (paddle_pin_read(paddle_left) == HIGH) && (paddle_pin_read(paddle_right) == HIGH))
  {
    iambic_flag = 0;
    dit_buffer = 0;
    dah_buffer = 0;
  }

  if ((being_sent == SENDING_DIT) || (being_sent == SENDING_DAH))
  {
    if (configuration.dit_buffer_off)
    {
      dit_buffer = 0;
    }
    if (configuration.dah_buffer_off)
    {
      dah_buffer = 0;
    }
  }

} //void loop_element_lengths

//-------------------------------------------------------------------------------------------------------


//-------------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------------

void speed_change(int change)
{
  if (((configuration.wpm + change) > wpm_limit_low) && ((configuration.wpm + change) < wpm_limit_high))
  {
    speed_set(configuration.wpm + change);
  }

#ifdef FEATURE_DISPLAY
  lcd_center_print_timed_wpm();
#endif
}

//-------------------------------------------------------------------------------------------------------

void speed_change_command_mode(int change)
{
  if (((configuration.wpm_command_mode + change) > wpm_limit_low) && ((configuration.wpm_command_mode + change) < wpm_limit_high))
  {
    configuration.wpm_command_mode = configuration.wpm_command_mode + change;
    config_dirty = 1;
  }

#ifdef FEATURE_DISPLAY
  lcd_center_print_timed(String(configuration.wpm_command_mode) + " wpm", 0, default_display_msg_delay);
#endif
}

//-------------------------------------------------------------------------------------------------------

long get_cw_input_from_user(unsigned int exit_time_milliseconds)
{

  byte looping = 1;
  byte paddle_hit = 0;
  long cw_char = 0;
  unsigned long last_element_time = 0;
  byte button_hit = 0;
  unsigned long entry_time = millis();

  while (looping)
  {

#ifdef OPTION_WATCHDOG_TIMER
    wdt_reset();
#endif //OPTION_WATCHDOG_TIMER

#ifdef FEATURE_POTENTIOMETER
    if (configuration.pot_activated)
    {
      check_potentiometer();
    }
#endif

#ifdef FEATURE_ROTARY_ENCODER
    check_rotary_encoder();
#endif //FEATURE_ROTARY_ENCODER

    check_paddles();

    if (dit_buffer)
    {
      sending_mode = MANUAL_SENDING;
      send_dit();
      dit_buffer = 0;
      paddle_hit = 1;
      cw_char = (cw_char * 10) + 1;
      last_element_time = millis();
    }
    if (dah_buffer)
    {
      sending_mode = MANUAL_SENDING;
      send_dah();
      dah_buffer = 0;
      paddle_hit = 1;
      cw_char = (cw_char * 10) + 2;
      last_element_time = millis();
    }
    if ((paddle_hit) && (millis() > (last_element_time + (float(600 / configuration.wpm) * length_letterspace))))
    {
#ifdef DEBUG_GET_CW_INPUT_FROM_USER
      debug_serial_port->println(F("get_cw_input_from_user: hit length_letterspace"));
#endif
      looping = 0;
    }

    if ((!paddle_hit) && (exit_time_milliseconds) && ((millis() - entry_time) > exit_time_milliseconds))
    { // if we were passed an exit time and no paddle was hit, blow out of here
      return 0;
    }

#ifdef FEATURE_COMMAND_BUTTONS
    while (analogbuttonread(0))
    { // hit the button to get out of command mode if no paddle was hit
      looping = 0;
      button_hit = 1;
    }
#endif

#if defined(FEATURE_SERIAL)
    check_serial();
#endif

  } //while (looping)

  if (button_hit)
  {
#ifdef DEBUG_GET_CW_INPUT_FROM_USER
    debug_serial_port->println(F("get_cw_input_from_user: button_hit exit 9"));
#endif
    return 9;
  }
  else
  {
#ifdef DEBUG_GET_CW_INPUT_FROM_USER
    debug_serial_port->print(F("get_cw_input_from_user: exiting cw_char:"));
    debug_serial_port->println(cw_char);
#endif
    return cw_char;
  }
}

//-------------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------------

// end command_display_memory

//-------------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------------

void adjust_dah_to_dit_ratio(int adjustment)
{

  if ((configuration.dah_to_dit_ratio + adjustment) > 150 && (configuration.dah_to_dit_ratio + adjustment) < 810)
  {
    configuration.dah_to_dit_ratio = configuration.dah_to_dit_ratio + adjustment;
#ifdef FEATURE_DISPLAY
#ifdef OPTION_MORE_DISPLAY_MSGS
    if (LCD_COLUMNS < 9)
    {
      lcd_center_print_timed("DDR:" + String(configuration.dah_to_dit_ratio), 0, default_display_msg_delay);
    }
    else
    {
      lcd_center_print_timed("Dah/Dit: " + String(configuration.dah_to_dit_ratio), 0, default_display_msg_delay);
    }
    service_display();
#endif
#endif
  }

  config_dirty = 1;
}

//-------------------------------------------------------------------------------------------------------

void sidetone_adj(int hz)
{

  if (((configuration.hz_sidetone + hz) > sidetone_hz_limit_low) && ((configuration.hz_sidetone + hz) < sidetone_hz_limit_high))
  {
    configuration.hz_sidetone = configuration.hz_sidetone + hz;
    config_dirty = 1;
#if defined(FEATURE_DISPLAY) && defined(OPTION_MORE_DISPLAY_MSGS)
    if (LCD_COLUMNS < 9)
    {
      lcd_center_print_timed(String(configuration.hz_sidetone) + " Hz", 0, default_display_msg_delay);
    }
    else
    {
      lcd_center_print_timed("Sidetone " + String(configuration.hz_sidetone) + " Hz", 0, default_display_msg_delay);
    }
#endif
  }
}

//-------------------------------------------------------------------------------------------------------

//------------------------------------------------------------------

void switch_to_tx_silent(byte tx)
{

  switch (tx)
  {
  case 1:
    if ((ptt_tx_1) || (tx_key_line_1))
    {
      configuration.current_ptt_line = ptt_tx_1;
      current_tx_key_line = tx_key_line_1;
      configuration.current_tx = 1;
      config_dirty = 1;
    }
    break;
  case 2:
    if ((ptt_tx_2) || (tx_key_line_2))
    {
      configuration.current_ptt_line = ptt_tx_2;
      current_tx_key_line = tx_key_line_2;
      configuration.current_tx = 2;
      config_dirty = 1;
    }
    break;
  case 3:
    if ((ptt_tx_3) || (tx_key_line_3))
    {
      configuration.current_ptt_line = ptt_tx_3;
      current_tx_key_line = tx_key_line_3;
      configuration.current_tx = 3;
      config_dirty = 1;
    }
    break;
  case 4:
    if ((ptt_tx_4) || (tx_key_line_4))
    {
      configuration.current_ptt_line = ptt_tx_4;
      current_tx_key_line = tx_key_line_4;
      configuration.current_tx = 4;
      config_dirty = 1;
    }
    break;
  case 5:
    if ((ptt_tx_5) || (tx_key_line_5))
    {
      configuration.current_ptt_line = ptt_tx_5;
      current_tx_key_line = tx_key_line_5;
      configuration.current_tx = 5;
      config_dirty = 1;
    }
    break;
  case 6:
    if ((ptt_tx_6) || (tx_key_line_6))
    {
      configuration.current_ptt_line = ptt_tx_6;
      current_tx_key_line = tx_key_line_6;
      configuration.current_tx = 6;
      config_dirty = 1;
    }
    break;
  }
}

//------------------------------------------------------------------
void switch_to_tx(byte tx)
{

#ifdef FEATURE_MEMORIES
  repeat_memory = 255;
#endif

#ifdef FEATURE_DISPLAY
  switch (tx)
  {
  case 1:
    if ((ptt_tx_1) || (tx_key_line_1))
    {
      switch_to_tx_silent(1);
      lcd_center_print_timed("TX 1", 0, default_display_msg_delay);
    }
    break;
  case 2:
    if ((ptt_tx_2) || (tx_key_line_2))
    {
      switch_to_tx_silent(2);
      lcd_center_print_timed("TX 2", 0, default_display_msg_delay);
    }
    break;
  case 3:
    if ((ptt_tx_3) || (tx_key_line_3))
    {
      switch_to_tx_silent(3);
      lcd_center_print_timed("TX 3", 0, default_display_msg_delay);
    }
    break;
  case 4:
    if ((ptt_tx_4) || (tx_key_line_4))
    {
      switch_to_tx_silent(4);
      lcd_center_print_timed("TX 4", 0, default_display_msg_delay);
    }
    break;
  case 5:
    if ((ptt_tx_5) || (tx_key_line_5))
    {
      switch_to_tx_silent(5);
      lcd_center_print_timed("TX 5", 0, default_display_msg_delay);
    }
    break;
  case 6:
    if ((ptt_tx_6) || (tx_key_line_6))
    {
      switch_to_tx_silent(6);
      lcd_center_print_timed("TX 6", 0, default_display_msg_delay);
    }
    break;
  }
#else
  switch (tx)
  {
  case 1:
    if ((ptt_tx_1) || (tx_key_line_1))
    {
      switch_to_tx_silent(1);
      send_tx();
      send_char('1', KEYER_NORMAL);
    }
    break;
  case 2:
    if ((ptt_tx_2) || (tx_key_line_2))
    {
      switch_to_tx_silent(2);
      send_tx();
      send_char('2', KEYER_NORMAL);
    }
    break;
  case 3:
    if ((ptt_tx_3) || (tx_key_line_3))
    {
      switch_to_tx_silent(3);
      send_tx();
      send_char('3', KEYER_NORMAL);
    }
    break;
  case 4:
    if ((ptt_tx_4) || (tx_key_line_4))
    {
      switch_to_tx_silent(4);
      send_tx();
      send_char('4', KEYER_NORMAL);
    }
    break;
  case 5:
    if ((ptt_tx_5) || (tx_key_line_5))
    {
      switch_to_tx_silent(5);
      send_tx();
      send_char('5', KEYER_NORMAL);
    }
    break;
  case 6:
    if ((ptt_tx_6) || (tx_key_line_6))
    {
      switch_to_tx_silent(6);
      send_tx();
      send_char('6', KEYER_NORMAL);
    }
    break;
  }
#endif
}

//------------------------------------------------------------------

//------------------------------------------------------------------

void service_dit_dah_buffers()
{
#ifdef DEBUG_LOOP
  debug_serial_port->println(F("loop: entering service_dit_dah_buffers"));
#endif

  if (keyer_machine_mode == BEACON)
  {
    return;
  }

  if (automatic_sending_interruption_time != 0)
  {
    if ((millis() - automatic_sending_interruption_time) > (configuration.paddle_interruption_quiet_time_element_lengths * (1200 / configuration.wpm)))
    {
      automatic_sending_interruption_time = 0;
      sending_mode = MANUAL_SENDING;
    }
    else
    {
      dit_buffer = 0;
      dah_buffer = 0;
      return;
    }
  }

  static byte bug_dah_flag = 0;

#ifdef FEATURE_PADDLE_ECHO
  static unsigned long bug_dah_key_down_time = 0;
#endif //FEATURE_PADDLE_ECHO

  if ((configuration.keyer_mode == IAMBIC_A) || (configuration.keyer_mode == IAMBIC_B) || (configuration.keyer_mode == ULTIMATIC) || (configuration.keyer_mode == SINGLE_PADDLE))
  {
    if ((configuration.keyer_mode == IAMBIC_A) && (iambic_flag) && (paddle_pin_read(paddle_left)) && (paddle_pin_read(paddle_right)))
    {
      iambic_flag = 0;
      dit_buffer = 0;
      dah_buffer = 0;
    }
    else
    {
      if (dit_buffer)
      {
        dit_buffer = 0;
        sending_mode = MANUAL_SENDING;
        //sendEspNowDitDah(ESPNOW_DIT);
        send_dit();
      }
      if (dah_buffer)
      {
        dah_buffer = 0;
        sending_mode = MANUAL_SENDING;
        //sendEspNowDitDah(ESPNOW_DAH);
        send_dah();
      }
    }
  }
  else
  {
    if (configuration.keyer_mode == BUG)
    {
      if (dit_buffer)
      {
        dit_buffer = 0;
        sending_mode = MANUAL_SENDING;
        send_dit();
      }

      if (dah_buffer)
      {
        dah_buffer = 0;
        if (!bug_dah_flag)
        {
          sending_mode = MANUAL_SENDING;
          tx_and_sidetone_key(1);
          bug_dah_flag = 1;
#ifdef FEATURE_PADDLE_ECHO
          bug_dah_key_down_time = millis();
#endif //FEATURE_PADDLE_ECHO
        }

#ifdef FEATURE_PADDLE_ECHO

        //zzzzzz

        //paddle_echo_buffer_decode_time = millis() + (float((cw_echo_timing_factor*3000.0)/configuration.wpm)*length_letterspace);
#endif //FEATURE_PADDLE_ECHO
      }
      else
      {
        if (bug_dah_flag)
        {
          sending_mode = MANUAL_SENDING;
          tx_and_sidetone_key(0);
#ifdef FEATURE_PADDLE_ECHO
          if ((millis() - bug_dah_key_down_time) > (0.5 * (1200.0 / configuration.wpm)))
          {
            if ((millis() - bug_dah_key_down_time) > (2 * (1200.0 / configuration.wpm)))
            {
              paddle_echo_buffer = (paddle_echo_buffer * 10) + 2;
            }
            else
            {
              paddle_echo_buffer = (paddle_echo_buffer * 10) + 1;
            }
            paddle_echo_buffer_decode_time = millis() + (float((cw_echo_timing_factor * 3000.0) / configuration.wpm) * length_letterspace);
          }
#endif //FEATURE_PADDLE_ECHO
          bug_dah_flag = 0;
        }
      }
#ifdef FEATURE_DEAD_OP_WATCHDOG
      dah_counter = 0;
#endif
    }
    else
    {
      if (configuration.keyer_mode == STRAIGHT)
      {
        if (dit_buffer)
        {
          dit_buffer = 0;
          sending_mode = MANUAL_SENDING;
          tx_and_sidetone_key(1);
        }
        else
        {
          sending_mode = MANUAL_SENDING;
          tx_and_sidetone_key(0);
        }
#ifdef FEATURE_DEAD_OP_WATCHDOG
        dit_counter = 0;
#endif
      }
    }
  }
}

//-------------------------------------------------------------------------------------------------------

void beep()
{
#if !defined(OPTION_SIDETONE_DIGITAL_OUTPUT_NO_SQUARE_WAVE)
  // #if defined(FEATURE_SINEWAVE_SIDETONE)
  //   tone(sidetone_line, hz_high_beep);
  //   delay(200);
  //   noTone(sidetone_line);
  // #else
  tone(sidetone_line, hz_high_beep, 200);
  // #endif
#else
  if (sidetone_line)
  {
    digitalWrite(sidetone_line, HIGH);
    delay(200);
    digitalWrite(sidetone_line, LOW);
  }
#endif
}

//-------------------------------------------------------------------------------------------------------

void boop()
{
#if !defined(OPTION_SIDETONE_DIGITAL_OUTPUT_NO_SQUARE_WAVE)
  tone(sidetone_line, hz_low_beep);
  delay(100);
  noTone(sidetone_line);
#else
  if (sidetone_line)
  {
    digitalWrite(sidetone_line, HIGH);
    delay(100);
    digitalWrite(sidetone_line, LOW);
  }
#endif
}

//-------------------------------------------------------------------------------------------------------

void beep_boop()
{
#if !defined(OPTION_SIDETONE_DIGITAL_OUTPUT_NO_SQUARE_WAVE)
  tone(sidetone_line, hz_high_beep);
  delay(100);
  tone(sidetone_line, hz_low_beep);
  delay(100);
  noTone(sidetone_line);
#else
  if (sidetone_line)
  {
    digitalWrite(sidetone_line, HIGH);
    delay(200);
    digitalWrite(sidetone_line, LOW);
  }
#endif
}

//-------------------------------------------------------------------------------------------------------

void boop_beep()
{
#if !defined(OPTION_SIDETONE_DIGITAL_OUTPUT_NO_SQUARE_WAVE)
  tone(sidetone_line, hz_low_beep);
  delay(100);
  tone(sidetone_line, hz_high_beep);
  delay(100);
  noTone(sidetone_line);
#else
  if (sidetone_line)
  {
    digitalWrite(sidetone_line, HIGH);
    delay(200);
    digitalWrite(sidetone_line, LOW);
  }
#endif
}

//-------------------------------------------------------------------------------------------------------
void send_the_dits_and_dahs(char const *cw_to_send)
{

  /* American Morse - Special Symbols

    ~  long dah (4 units)

    =  very long dah (5 units)

    &  an extra space (1 unit)

  */

  //debug_serial_port->println(F("send_the_dits_and_dahs()"));

  sending_mode = AUTOMATIC_SENDING;

#if defined(FEATURE_SERIAL) && !defined(OPTION_DISABLE_SERIAL_PORT_CHECKING_WHILE_SENDING_CW)
  dump_current_character_flag = 0;
#endif

#if defined(FEATURE_FARNSWORTH)
  float additional_intercharacter_time_ms;
#endif

  for (int x = 0; x < 12; x++)
  {
    switch (cw_to_send[x])
    {
    case '.':
      send_dit();
      break;
    case '-':
      send_dah();
      break;
#if defined(FEATURE_AMERICAN_MORSE) // this is a bit of a hack, but who cares!  :-)
    case '~':

      being_sent = SENDING_DAH;
      tx_and_sidetone_key(1);
      if ((tx_key_dah) && (key_tx))
      {
        digitalWrite(tx_key_dah, tx_key_dit_and_dah_pins_active_state);
      }
      loop_element_lengths((float(4.0) * (float(configuration.weighting) / 50)), keying_compensation, configuration.wpm);
      if ((tx_key_dah) && (key_tx))
      {
        digitalWrite(tx_key_dah, tx_key_dit_and_dah_pins_inactive_state);
      }
      tx_and_sidetone_key(0);
      loop_element_lengths((4.0 - (3.0 * (float(configuration.weighting) / 50))), (-1.0 * keying_compensation), configuration.wpm);
      break;

    case '=':
      being_sent = SENDING_DAH;
      tx_and_sidetone_key(1);
      if ((tx_key_dah) && (key_tx))
      {
        digitalWrite(tx_key_dah, tx_key_dit_and_dah_pins_active_state);
      }
      loop_element_lengths((float(5.0) * (float(configuration.weighting) / 50)), keying_compensation, configuration.wpm);
      if ((tx_key_dah) && (key_tx))
      {
        digitalWrite(tx_key_dah, tx_key_dit_and_dah_pins_inactive_state);
      }
      tx_and_sidetone_key(0);
      loop_element_lengths((4.0 - (3.0 * (float(configuration.weighting) / 50))), (-1.0 * keying_compensation), configuration.wpm);
      break;

    case '&':
      loop_element_lengths((4.0 - (3.0 * (float(configuration.weighting) / 50))), (-1.0 * keying_compensation), configuration.wpm);
      break;
#endif //FEATURE_AMERICAN_MORSE
    default:
      //return;
      x = 12;
      break;
    }

    if ((dit_buffer || dah_buffer || sending_mode == AUTOMATIC_SENDING_INTERRUPTED) && (keyer_machine_mode != BEACON))
    {
      dit_buffer = 0;
      dah_buffer = 0;
      //debug_serial_port->println(F("send_the_dits_and_dahs: AUTOMATIC_SENDING_INTERRUPTED"));
      //return;
      x = 12;
    }
#if defined(FEATURE_SERIAL)
    check_serial();
#if !defined(OPTION_DISABLE_SERIAL_PORT_CHECKING_WHILE_SENDING_CW)
    if (dump_current_character_flag)
    {
      x = 12;
    }
#endif
#endif

#ifdef OPTION_WATCHDOG_TIMER
    wdt_reset();
#endif //OPTION_WATCHDOG_TIMER

  } // for (int x = 0;x < 12;x++)
}

//-------------------------------------------------------------------------------------------------------

void send_char(byte cw_char, byte omit_letterspace)
{
#ifdef DEBUG_SEND_CHAR
  debug_serial_port->print(F("send_char: called with cw_char:"));
  debug_serial_port->print((byte)cw_char);
  if (omit_letterspace)
  {
    debug_serial_port->print(F(" OMIT_LETTERSPACE"));
  }
  debug_serial_port->println();
#endif

#ifdef FEATURE_SLEEP
  last_activity_time = millis();
#endif //FEATURE_SLEEP

  if ((cw_char == 10) || (cw_char == 13))
  {
    return;
  } // don't attempt to send carriage return or line feed

  sending_mode = AUTOMATIC_SENDING;

  if (char_send_mode == CW)
  {
    switch (cw_char)
    {
    case 'A':
      send_the_dits_and_dahs(".-");
      break;
    case 'B':
      send_the_dits_and_dahs("-...");
      break;
    case 'C':
      send_the_dits_and_dahs("-.-.");
      break;
    case 'D':
      send_the_dits_and_dahs("-..");
      break;
    case 'E':
      send_the_dits_and_dahs(".");
      break;
    case 'F':
      send_the_dits_and_dahs("..-.");
      break;
    case 'G':
      send_the_dits_and_dahs("--.");
      break;
    case 'H':
      send_the_dits_and_dahs("....");
      break;
    case 'I':
      send_the_dits_and_dahs("..");
      break;
    case 'J':
      send_the_dits_and_dahs(".---");
      break;
    case 'K':
      send_the_dits_and_dahs("-.-");
      break;
    case 'L':
      send_the_dits_and_dahs(".-..");
      break;
    case 'M':
      send_the_dits_and_dahs("--");
      break;
    case 'N':
      send_the_dits_and_dahs("-.");
      break;
    case 'O':
      send_the_dits_and_dahs("---");
      break;
    case 'P':
      send_the_dits_and_dahs(".--.");
      break;
    case 'Q':
      send_the_dits_and_dahs("--.-");
      break;
    case 'R':
      send_the_dits_and_dahs(".-.");
      break;
    case 'S':
      send_the_dits_and_dahs("...");
      break;
    case 'T':
      send_the_dits_and_dahs("-");
      break;
    case 'U':
      send_the_dits_and_dahs("..-");
      break;
    case 'V':
      send_the_dits_and_dahs("...-");
      break;
    case 'W':
      send_the_dits_and_dahs(".--");
      break;
    case 'X':
      send_the_dits_and_dahs("-..-");
      break;
    case 'Y':
      send_the_dits_and_dahs("-.--");
      break;
    case 'Z':
      send_the_dits_and_dahs("--..");
      break;

    case '0':
      send_the_dits_and_dahs("-----");
      break;
    case '1':
      send_the_dits_and_dahs(".----");
      break;
    case '2':
      send_the_dits_and_dahs("..---");
      break;
    case '3':
      send_the_dits_and_dahs("...--");
      break;
    case '4':
      send_the_dits_and_dahs("....-");
      break;
    case '5':
      send_the_dits_and_dahs(".....");
      break;
    case '6':
      send_the_dits_and_dahs("-....");
      break;
    case '7':
      send_the_dits_and_dahs("--...");
      break;
    case '8':
      send_the_dits_and_dahs("---..");
      break;
    case '9':
      send_the_dits_and_dahs("----.");
      break;

    case '=':
      send_the_dits_and_dahs("-...-");
      break;
    case '/':
      send_the_dits_and_dahs("-..-.");
      break;
    case ' ':
      loop_element_lengths((configuration.length_wordspace - length_letterspace - 2), 0, configuration.wpm);
      break;
    case '*':
      send_the_dits_and_dahs("-...-.-");
      break;
    //case '&': send_dit(); loop_element_lengths(3); send_dits(3); break;
    case '.':
      send_the_dits_and_dahs(".-.-.-");
      break;
    case ',':
      send_the_dits_and_dahs("--..--");
      break;
    case '!':
      send_the_dits_and_dahs("--..--");
      break; //sp5iou 20180328
    case '\'':
      send_the_dits_and_dahs(".----.");
      break; // apostrophe
             //      case '!': send_the_dits_and_dahs("-.-.--");break;//sp5iou 20180328
    case '(':
      send_the_dits_and_dahs("-.--.");
      break;
    case ')':
      send_the_dits_and_dahs("-.--.-");
      break;
    case '&':
      send_the_dits_and_dahs(".-...");
      break;
    case ':':
      send_the_dits_and_dahs("---...");
      break;
    case ';':
      send_the_dits_and_dahs("-.-.-.");
      break;
    case '+':
      send_the_dits_and_dahs(".-.-.");
      break;
    case '-':
      send_the_dits_and_dahs("-....-");
      break;
    case '_':
      send_the_dits_and_dahs("..--.-");
      break;
    case '"':
      send_the_dits_and_dahs(".-..-.");
      break;
    case '$':
      send_the_dits_and_dahs("...-..-");
      break;
    case '@':
      send_the_dits_and_dahs(".--.-.");
      break;
    case '<':
      send_the_dits_and_dahs(".-.-.");
      break; // AR
    case '>':
      send_the_dits_and_dahs("...-.-");
      break; // SK

#ifdef OPTION_RUSSIAN_LANGUAGE_SEND_CLI // Contributed by Павел Бирюков, UA1AQC
    case 192:
      send_the_dits_and_dahs(".-");
      break; //А
    case 193:
      send_the_dits_and_dahs("-...");
      break; //Б
    case 194:
      send_the_dits_and_dahs(".--");
      break; //В
    case 195:
      send_the_dits_and_dahs("--.");
      break; //Г
    case 196:
      send_the_dits_and_dahs("-..");
      break; //Д
    case 197:
      send_the_dits_and_dahs(".");
      break; //Е
    case 168:
      send_the_dits_and_dahs(".");
      break; //Ё
    case 184:
      send_the_dits_and_dahs(".");
      break; //ё
    case 198:
      send_the_dits_and_dahs("...-");
      break; //Ж
    case 199:
      send_the_dits_and_dahs("--..");
      break; //З
    case 200:
      send_the_dits_and_dahs("..");
      break; //И
    case 201:
      send_the_dits_and_dahs(".---");
      break; //Й
    case 202:
      send_the_dits_and_dahs("-.-");
      break; //К
    case 203:
      send_the_dits_and_dahs(".-..");
      break; //Л
    case 204:
      send_the_dits_and_dahs("--");
      break; //М
    case 205:
      send_the_dits_and_dahs("-.");
      break; //Н
    case 206:
      send_the_dits_and_dahs("---");
      break; //О
    case 207:
      send_the_dits_and_dahs(".--.");
      break; //П
    case 208:
      send_the_dits_and_dahs(".-.");
      break; //Р
    case 209:
      send_the_dits_and_dahs("...");
      break; //С
    case 210:
      send_the_dits_and_dahs("-");
      break; //Т
    case 211:
      send_the_dits_and_dahs("..-");
      break; //У
    case 212:
      send_the_dits_and_dahs("..-.");
      break; //Ф
    case 213:
      send_the_dits_and_dahs("....");
      break; //Х
    case 214:
      send_the_dits_and_dahs("-.-.");
      break; //Ц
    case 215:
      send_the_dits_and_dahs("---.");
      break; //Ч
    case 216:
      send_the_dits_and_dahs("----");
      break; //Ш
    case 217:
      send_the_dits_and_dahs("--.-");
      break; //Щ
    case 218:
      send_the_dits_and_dahs("--.--");
      break; //Ъ
    case 219:
      send_the_dits_and_dahs("-.--");
      break; //Ы
    case 220:
      send_the_dits_and_dahs("-..-");
      break; //Ь
    case 221:
      send_the_dits_and_dahs("..-..");
      break; //Э
    case 222:
      send_the_dits_and_dahs("..--");
      break; //Ю
    case 223:
      send_the_dits_and_dahs(".-.-");
      break; //Я
    case 255:
      send_the_dits_and_dahs(".-.-");
      break; //я
#endif       //OPTION_RUSSIAN_LANGUAGE_SEND_CLI

    case '\n':
      break;
    case '\r':
      break;

#if defined(OPTION_PROSIGN_SUPPORT)
    case PROSIGN_AA:
      send_the_dits_and_dahs(".-.-");
      break;
    case PROSIGN_AS:
      send_the_dits_and_dahs(".-...");
      break;
    case PROSIGN_BK:
      send_the_dits_and_dahs("-...-.-");
      break;
    case PROSIGN_CL:
      send_the_dits_and_dahs("-.-..-..");
      break;
    case PROSIGN_CT:
      send_the_dits_and_dahs("-.-.-");
      break;
    case PROSIGN_KN:
      send_the_dits_and_dahs("-.--.");
      break;
    case PROSIGN_NJ:
      send_the_dits_and_dahs("-..---");
      break;
    case PROSIGN_SK:
      send_the_dits_and_dahs("...-.-");
      break;
    case PROSIGN_SN:
      send_the_dits_and_dahs("...-.");
      break;
    case PROSIGN_HH:
      send_the_dits_and_dahs("........");
      break; // iz0rus
#endif

#ifdef OPTION_NON_ENGLISH_EXTENSIONS
    case 192:
      send_the_dits_and_dahs(".--.-");
      break; // 'À'
    case 194:
      send_the_dits_and_dahs(".-.-");
      break; // 'Â'
    case 197:
      send_the_dits_and_dahs(".--.-");
      break; // 'Å'
    case 196:
      send_the_dits_and_dahs(".-.-");
      break; // 'Ä'
    case 198:
      send_the_dits_and_dahs(".-.-");
      break; // 'Æ'
    case 199:
      send_the_dits_and_dahs("-.-..");
      break; // 'Ç'
    case 208:
      send_the_dits_and_dahs("..--.");
      break; // 'Ð'
    case 138:
      send_the_dits_and_dahs("----");
      break; // 'Š'
    case 200:
      send_the_dits_and_dahs(".-..-");
      break; // 'È'
    case 201:
      send_the_dits_and_dahs("..-..");
      break; // 'É'
    case 142:
      send_the_dits_and_dahs("--..-.");
      break; // 'Ž'
    case 209:
      send_the_dits_and_dahs("--.--");
      break; // 'Ñ'
    case 214:
      send_the_dits_and_dahs("---.");
      break; // 'Ö'
    case 216:
      send_the_dits_and_dahs("---.");
      break; // 'Ø'
    case 211:
      send_the_dits_and_dahs("---.");
      break; // 'Ó'
    case 220:
      send_the_dits_and_dahs("..--");
      break; // 'Ü'
    case 223:
      send_the_dits_and_dahs("------");
      break; // 'ß'

    // for English/Japanese font LCD controller which has a few European characters also (HD44780UA00) (LA3ZA code)
    case 225:
      send_the_dits_and_dahs(".-.-");
      break; // 'ä' LA3ZA
    case 239:
      send_the_dits_and_dahs("---.");
      break; // 'ö' LA3ZA
    case 242:
      send_the_dits_and_dahs("---.");
      break; // 'ø' LA3ZA
    case 245:
      send_the_dits_and_dahs("..--");
      break; // 'ü' LA3ZA
    case 246:
      send_the_dits_and_dahs("----");
      break; // almost '' or rather sigma LA3ZA
    case 252:
      send_the_dits_and_dahs(".--.-");
      break; // å (sort of) LA3ZA
    case 238:
      send_the_dits_and_dahs("--.--");
      break; // 'ñ' LA3ZA
    case 226:
      send_the_dits_and_dahs("------");
      break; // 'ß' LA3ZA
#endif       //OPTION_NON_ENGLISH_EXTENSIONS

    case '|':
#if !defined(OPTION_WINKEY_DO_NOT_SEND_7C_BYTE_HALF_SPACE)
      loop_element_lengths(0.5, 0, configuration.wpm);
#endif
      return;
      break;

#if defined(OPTION_DO_NOT_SEND_UNKNOWN_CHAR_QUESTION)
    case '?':
      send_the_dits_and_dahs("..--..");
      break;
#endif

    default:
#if !defined(OPTION_DO_NOT_SEND_UNKNOWN_CHAR_QUESTION)
      send_the_dits_and_dahs("..--..");
#endif
      break;
    }
    if (omit_letterspace != OMIT_LETTERSPACE)
    {

      loop_element_lengths((length_letterspace - 1), 0, configuration.wpm); //this is minus one because send_dit and send_dah have a trailing element space
    }

#ifdef FEATURE_FARNSWORTH
    // Farnsworth Timing : http://www.arrl.org/files/file/Technology/x9004008.pdf
    if (configuration.wpm_farnsworth > configuration.wpm)
    {
      float additional_intercharacter_time_ms = ((((1.0 * farnsworth_timing_calibration) * ((60.0 * float(configuration.wpm_farnsworth)) - (37.2 * float(configuration.wpm))) / (float(configuration.wpm) * float(configuration.wpm_farnsworth))) / 19.0) * 1000.0) - (1200.0 / float(configuration.wpm_farnsworth));
      loop_element_lengths(1, additional_intercharacter_time_ms, 0);
    }
#endif
  }
  else
  {
    if (char_send_mode == HELL)
    {
#ifdef FEATURE_HELL
      transmit_hell_char(cw_char);
#endif
    }
    else
    {
      if (char_send_mode == AMERICAN_MORSE)
      {
#ifdef FEATURE_AMERICAN_MORSE

        /* 

            ~  long dah (4 units)
    
            =  very long dah (5 units)
      
            &  an extra space (1 unit)

          */

        switch (cw_char)
        { // THIS SECTION IS AMERICAN MORSE CODE - DO NOT TOUCH IT !

        case 'A':
          send_the_dits_and_dahs(".-");
          break;
        case 'B':
          send_the_dits_and_dahs("-...");
          break;
        case 'C':
          send_the_dits_and_dahs("..&.");
          break;
        case 'D':
          send_the_dits_and_dahs("-..");
          break;
        case 'E':
          send_the_dits_and_dahs(".");
          break;
        case 'F':
          send_the_dits_and_dahs(".-.");
          break;
        case 'G':
          send_the_dits_and_dahs("--.");
          break;
        case 'H':
          send_the_dits_and_dahs("....");
          break;
        case 'I':
          send_the_dits_and_dahs("..");
          break;
        case 'J':
          send_the_dits_and_dahs("-.-.");
          break;
        case 'K':
          send_the_dits_and_dahs("-.-");
          break;
        case 'L':
          send_the_dits_and_dahs("~");
          break;
        case 'M':
          send_the_dits_and_dahs("--");
          break;
        case 'N':
          send_the_dits_and_dahs("-.");
          break;
        case 'O':
          send_the_dits_and_dahs(".&.");
          break;
        case 'P':
          send_the_dits_and_dahs(".....");
          break;
        case 'Q':
          send_the_dits_and_dahs("..-.");
          break;
        case 'R':
          send_the_dits_and_dahs(".&..");
          break;
        case 'S':
          send_the_dits_and_dahs("...");
          break;
        case 'T':
          send_the_dits_and_dahs("-");
          break;
        case 'U':
          send_the_dits_and_dahs("..-");
          break;
        case 'V':
          send_the_dits_and_dahs("...-");
          break;
        case 'W':
          send_the_dits_and_dahs(".--");
          break;
        case 'X':
          send_the_dits_and_dahs(".-..");
          break;
        case 'Y':
          send_the_dits_and_dahs("..&..");
          break;
        case 'Z':
          send_the_dits_and_dahs("...&.");
          break;

          // THIS SECTION IS AMERICAN MORSE CODE - DO NOT TOUCH IT !

        case '&':
          send_the_dits_and_dahs(".&...");
          break;

        case '0':
          send_the_dits_and_dahs("=");
          break;
        case '1':
          send_the_dits_and_dahs(".---.");
          break;
        case '2':
          send_the_dits_and_dahs("..--..");
          break;
        case '3':
          send_the_dits_and_dahs("...-.");
          break;
        case '4':
          send_the_dits_and_dahs("....-");
          break;
        case '5':
          send_the_dits_and_dahs("---");
          break;
        case '6':
          send_the_dits_and_dahs("......");
          break;
        case '7':
          send_the_dits_and_dahs("--..");
          break;
        case '8':
          send_the_dits_and_dahs("-....");
          break;
        case '9':
          send_the_dits_and_dahs("-..-");
          break;

          // THIS SECTION IS AMERICAN MORSE CODE - DO NOT TOUCH IT !

        case ',':
          send_the_dits_and_dahs(".-.-");
          break;
        case '.':
          send_the_dits_and_dahs("..--..");
          break;
        case '?':
          send_the_dits_and_dahs("-..-.");
          break;
        case '!':
          send_the_dits_and_dahs("---.");
          break;
        case ':':
          send_the_dits_and_dahs("-.-&.&.");
          break;
        case ';':
          send_the_dits_and_dahs("...&..");
          break;
        case '-':
          send_the_dits_and_dahs("....&.-..");
          break;

        } //switch (cw_char)

#endif
      }
    }
  }
}

//-------------------------------------------------------------------------------------------------------

int uppercase(int charbytein)
{
  if (((charbytein > 96) && (charbytein < 123)) || ((charbytein > 223) && (charbytein < 255)))
  {
    charbytein = charbytein - 32;
  }
  if (charbytein == 158)
  {
    charbytein = 142;
  } // ž -> Ž
  if (charbytein == 154)
  {
    charbytein = 138;
  } // š -> Š

  return charbytein;
}

//-------------------------------------------------------------------------------------------------------
#if defined(FEATURE_SERIAL)
#ifdef FEATURE_COMMAND_LINE_INTERFACE
void serial_qrss_mode()
{
  byte looping = 1;
  byte incoming_serial_byte;
  byte numbers[4];
  byte numberindex = 0;
  String numberstring;
  byte error = 0;

  while (looping)
  {
    if (primary_serial_port->available() == 0)
    { // wait for the next keystroke
      if (keyer_machine_mode == KEYER_NORMAL)
      { // might as well do something while we're waiting
        check_paddles();
        service_dit_dah_buffers();
        //check_the_memory_buttons();
      }
    }
    else
    {

      incoming_serial_byte = primary_serial_port->read();
      if ((incoming_serial_byte > 47) && (incoming_serial_byte < 58))
      { // ascii 48-57 = "0" - "9")
        numberstring = numberstring + incoming_serial_byte;
        numbers[numberindex] = incoming_serial_byte;
        //        primary_serial_port->write("numberindex:");
        //        primary_serial_port->print(numberindex,DEC);
        //        primary_serial_port->write("     numbers:");
        //        primary_serial_port->println(numbers[numberindex],DEC);
        numberindex++;
        if (numberindex > 2)
        {
          looping = 0;
          error = 1;
        }
      }
      else
      {
        if (incoming_serial_byte == 13)
        { // carriage return - get out
          looping = 0;
        }
        else
        { // bogus input - error out
          looping = 0;
          error = 1;
        }
      }
    }
  }

  if (error)
  {
    primary_serial_port->println(F("Error..."));
    while (primary_serial_port->available() > 0)
    {
      incoming_serial_byte = primary_serial_port->read();
    } // clear out buffer
    return;
  }
  else
  {
    primary_serial_port->print(F("Setting keyer to QRSS Mode. Dit length: "));
    primary_serial_port->print(numberstring);
    primary_serial_port->println(F(" seconds"));
    int y = 1;
    int set_dit_length = 0;
    for (int x = (numberindex - 1); x >= 0; x = x - 1)
    {
      set_dit_length = set_dit_length + ((numbers[x] - 48) * y);
      y = y * 10;
    }
    qrss_dit_length = set_dit_length;
    speed_mode = SPEED_QRSS;
  }
}
#endif
#endif
//-------------------------------------------------------------------------------------------------------

void service_send_buffer(byte no_print)
{
  // send one character out of the send buffer

#ifdef DEBUG_LOOP
  debug_serial_port->println(F("loop: entering service_send_buffer"));
#endif

  static unsigned long timed_command_end_time;
  static byte timed_command_in_progress = 0;

#if defined(DEBUG_SERVICE_SEND_BUFFER)

  byte no_bytes_flag = 0;

  if (send_buffer_bytes > 0)
  {
    debug_serial_port->print("service_send_buffer: enter:");
    for (int x = 0; x < send_buffer_bytes; x++)
    {
      debug_serial_port->write(send_buffer_array[x]);
      debug_serial_port->print("[");
      debug_serial_port->print(send_buffer_array[x]);
      debug_serial_port->print("]");
    }
    debug_serial_port->println();
  }
  else
  {
    no_bytes_flag = 1;
  }
  Serial.flush();
#endif

  if (service_tx_inhibit_and_pause() == 1)
  {
#if defined(DEBUG_SERVICE_SEND_BUFFER)
    debug_serial_port->println("service_send_buffer: tx_inhib");
#endif
    return;
  }

#ifdef FEATURE_MEMORIES
  play_memory_prempt = 0;
#endif

  if (send_buffer_status == SERIAL_SEND_BUFFER_NORMAL)
  {
    if ((send_buffer_bytes) && (pause_sending_buffer == 0))
    {
#ifdef FEATURE_SLEEP
      last_activity_time = millis();
#endif //FEATURE_SLEEP

      if ((send_buffer_array[0] > SERIAL_SEND_BUFFER_SPECIAL_START) && (send_buffer_array[0] < SERIAL_SEND_BUFFER_SPECIAL_END))
      {

#if defined(DEBUG_SERVICE_SEND_BUFFER)
        debug_serial_port->println("service_send_buffer: SERIAL_SEND_BUFFER_SPECIAL");
#endif

        if (send_buffer_array[0] == SERIAL_SEND_BUFFER_HOLD_SEND)
        {

#if defined(DEBUG_SERVICE_SEND_BUFFER)
          debug_serial_port->println("service_send_buffer: SERIAL_SEND_BUFFER_HOLD_SEND");
#endif

          send_buffer_status = SERIAL_SEND_BUFFER_HOLD;
          remove_from_send_buffer();
        }

        if (send_buffer_array[0] == SERIAL_SEND_BUFFER_HOLD_SEND_RELEASE)
        {

#if defined(DEBUG_SERVICE_SEND_BUFFER)
          debug_serial_port->println("service_send_buffer: SERIAL_SEND_BUFFER_HOLD_SEND_RELEASE");
#endif

          remove_from_send_buffer();
        }

        if (send_buffer_array[0] == SERIAL_SEND_BUFFER_MEMORY_NUMBER)
        {

#if defined(DEBUG_SERVICE_SEND_BUFFER)
          debug_serial_port->println("service_send_buffer: SERIAL_SEND_BUFFER_MEMORY_NUMBER");
#endif

#ifdef DEBUG_SEND_BUFFER
          debug_serial_port->println(F("service_send_buffer: SERIAL_SEND_BUFFER_MEMORY_NUMBER"));
#endif
#ifdef FEATURE_WINKEY_EMULATION
          if (winkey_sending && winkey_host_open)
          {
#if !defined(OPTION_WINKEY_UCXLOG_SUPRESS_C4_STATUS_BYTE)
            winkey_port_write(0xc0 | winkey_sending | winkey_xoff, 0);
#endif
            winkey_interrupted = 1;
          }
#endif
          remove_from_send_buffer();
          if (send_buffer_bytes)
          {
            if (send_buffer_array[0] < number_of_memories)
            {
#ifdef FEATURE_MEMORIES
              play_memory(send_buffer_array[0]);
#endif
            }
            remove_from_send_buffer();
          }
        }

        if (send_buffer_array[0] == SERIAL_SEND_BUFFER_WPM_CHANGE)
        { // two bytes for wpm
//remove_from_send_buffer();
#if defined(DEBUG_SERVICE_SEND_BUFFER)
          debug_serial_port->println("service_send_buffer: SERIAL_SEND_BUFFER_WPM_CHANGE");
#endif
          if (send_buffer_bytes > 2)
          {
#if defined(DEBUG_SERVICE_SEND_BUFFER)
            debug_serial_port->println("service_send_buffer: SERIAL_SEND_BUFFER_WPM_CHANGE: send_buffer_bytes>2");
#endif
            remove_from_send_buffer();
#ifdef FEATURE_WINKEY_EMULATION
            if ((winkey_host_open) && (winkey_speed_state == WINKEY_UNBUFFERED_SPEED))
            {
              winkey_speed_state = WINKEY_BUFFERED_SPEED;
              winkey_last_unbuffered_speed_wpm = configuration.wpm;
            }
#endif
            configuration.wpm = send_buffer_array[0] * 256;
            remove_from_send_buffer();
            configuration.wpm = configuration.wpm + send_buffer_array[0];
            remove_from_send_buffer();

#ifdef FEATURE_LED_RING
            update_led_ring();
#endif //FEATURE_LED_RING
          }
          else
          {
#if defined(DEBUG_SERVICE_SEND_BUFFER)
            debug_serial_port->println("service_send_buffer:SERIAL_SEND_BUFFER_WPM_CHANGE < 2");
#endif
          }

#if defined(DEBUG_SERVICE_SEND_BUFFER)
          debug_serial_port->print("service_send_buffer: SERIAL_SEND_BUFFER_WPM_CHANGE: exit send_buffer_bytes:");
          debug_serial_port->println(send_buffer_bytes);
#endif
        }

        if (send_buffer_array[0] == SERIAL_SEND_BUFFER_TX_CHANGE)
        { // one byte for transmitter #

#if defined(DEBUG_SERVICE_SEND_BUFFER)
          debug_serial_port->println("service_send_buffer: SERIAL_SEND_BUFFER_TX_CHANGE");
#endif

          remove_from_send_buffer();
          if (send_buffer_bytes > 1)
          {
// if ((send_buffer_array[0] > 0) && (send_buffer_array[0] < 7)){
//   switch_to_tx_silent(send_buffer_array[0]);
// }
#ifdef FEATURE_SO2R_BASE
            if ((send_buffer_array[0] > 0) && (send_buffer_array[0] < 3))
            {
              if (ptt_line_activated)
              {
                so2r_pending_tx = send_buffer_array[0];
              }
              else
              {
                so2r_tx = send_buffer_array[0];
                so2r_set_tx();
              }
            }
#else
            if ((send_buffer_array[0] > 0) && (send_buffer_array[0] < 7))
            {
              switch_to_tx_silent(send_buffer_array[0]);
            }
#endif //FEATURE_SO2R_BASE

            remove_from_send_buffer();
          }
        }

        if (send_buffer_array[0] == SERIAL_SEND_BUFFER_NULL)
        {

#if defined(DEBUG_SERVICE_SEND_BUFFER)
          debug_serial_port->println("service_send_buffer: SERIAL_SEND_BUFFER_NULL");
#endif

          remove_from_send_buffer();
        }

        if (send_buffer_array[0] == SERIAL_SEND_BUFFER_PROSIGN)
        {

#if defined(DEBUG_SERVICE_SEND_BUFFER)
          debug_serial_port->println("service_send_buffer: SERIAL_SEND_BUFFER_PROSIGN");
#endif

          remove_from_send_buffer();
          if (send_buffer_bytes)
          {
            send_char(send_buffer_array[0], OMIT_LETTERSPACE);
#ifdef FEATURE_WINKEY_EMULATION
            if (winkey_host_open)
            {
              // Must echo back PROSIGN characters sent  N6TV
              winkey_port_write(0xc4 | winkey_sending | winkey_xoff, 0); // N6TV
              winkey_port_write(send_buffer_array[0], 0);                // N6TV
            }
#endif //FEATURE_WINKEY_EMULATION
            remove_from_send_buffer();
          }
          if (send_buffer_bytes)
          {
            send_char(send_buffer_array[0], KEYER_NORMAL);
#ifdef FEATURE_WINKEY_EMULATION
            if (winkey_host_open)
            {
              // Must echo back PROSIGN characters sent  N6TV
              winkey_port_write(0xc4 | winkey_sending | winkey_xoff, 0); // N6TV
              winkey_port_write(send_buffer_array[0], 0);                // N6TV
            }
#endif //FEATURE_WINKEY_EMULATION
            remove_from_send_buffer();
          }
        }

        if (send_buffer_array[0] == SERIAL_SEND_BUFFER_TIMED_KEY_DOWN)
        {

#if defined(DEBUG_SERVICE_SEND_BUFFER)
          debug_serial_port->println("service_send_buffer: SERIAL_SEND_BUFFER_TIMED_KEY_DOWN");
#endif

          remove_from_send_buffer();
          if (send_buffer_bytes)
          {
            send_buffer_status = SERIAL_SEND_BUFFER_TIMED_COMMAND;
            sending_mode = AUTOMATIC_SENDING;
            tx_and_sidetone_key(1);
            timed_command_end_time = millis() + (send_buffer_array[0] * 1000);
            timed_command_in_progress = SERIAL_SEND_BUFFER_TIMED_KEY_DOWN;
            remove_from_send_buffer();
          }
        }

        if (send_buffer_array[0] == SERIAL_SEND_BUFFER_TIMED_WAIT)
        {

#if defined(DEBUG_SERVICE_SEND_BUFFER)
          debug_serial_port->println("service_send_buffer: SERIAL_SEND_BUFFER_TIMED_WAIT");
#endif

          remove_from_send_buffer();
          if (send_buffer_bytes)
          {
            send_buffer_status = SERIAL_SEND_BUFFER_TIMED_COMMAND;
            timed_command_end_time = millis() + (send_buffer_array[0] * 1000);
            timed_command_in_progress = SERIAL_SEND_BUFFER_TIMED_WAIT;
            remove_from_send_buffer();
          }
        }

        if (send_buffer_array[0] == SERIAL_SEND_BUFFER_PTT_ON)
        {

#if defined(DEBUG_SERVICE_SEND_BUFFER)
          debug_serial_port->println("service_send_buffer: SERIAL_SEND_BUFFER_PTT_ON");
#endif

          remove_from_send_buffer();
          manual_ptt_invoke = 1;
          ptt_key();
        }

        if (send_buffer_array[0] == SERIAL_SEND_BUFFER_PTT_OFF)
        {

#if defined(DEBUG_SERVICE_SEND_BUFFER)
          debug_serial_port->println("service_send_buffer: SERIAL_SEND_BUFFER_PTT_OFF");
#endif

          remove_from_send_buffer();
          manual_ptt_invoke = 0;
          ptt_unkey();
        }
      }
      else
      { // if ((send_buffer_array[0] > SERIAL_SEND_BUFFER_SPECIAL_START) && (send_buffer_array[0] < SERIAL_SEND_BUFFER_SPECIAL_END))

#ifdef FEATURE_WINKEY_EMULATION
        if ((primary_serial_port_mode == SERIAL_WINKEY_EMULATION) && (winkey_serial_echo) && (winkey_host_open) && (!no_print) && (!cw_send_echo_inhibit))
        {
#if defined(OPTION_WINKEY_ECHO_7C_BYTE)
          if (send_buffer_array[0] > 30)
          {
            winkey_port_write(send_buffer_array[0], 0);
          }
#else
          if ((send_buffer_array[0] != 0x7C) && (send_buffer_array[0] > 30))
          {
            winkey_port_write(send_buffer_array[0], 0);
          }
#endif
        }
#endif //FEATURE_WINKEY_EMULATION

#if defined(FEATURE_COMMAND_LINE_INTERFACE)
        if ((!no_print) && (!cw_send_echo_inhibit))
        {
          if (primary_serial_port_mode == SERIAL_CLI)
          {
            primary_serial_port->write(send_buffer_array[0]);
          };
#ifdef FEATURE_COMMAND_LINE_INTERFACE_ON_SECONDARY_PORT
          secondary_serial_port->write(send_buffer_array[0]);
#endif //FEATURE_COMMAND_LINE_INTERFACE_ON_SECONDARY_PORT
          if (send_buffer_array[0] == 13)
          {
            if (primary_serial_port_mode == SERIAL_CLI)
            {
              primary_serial_port->write(10);
            } // if we got a carriage return, also send a line feed
#ifdef FEATURE_COMMAND_LINE_INTERFACE_ON_SECONDARY_PORT
            secondary_serial_port->write(10);
#endif //FEATURE_COMMAND_LINE_INTERFACE_ON_SECONDARY_PORT
          }
        }
#endif //FEATURE_COMMAND_LINE_INTERFACE

#ifdef FEATURE_DISPLAY
        if (lcd_send_echo)
        {
          display_scroll_print_char(send_buffer_array[0]);
          service_display();
        }
#endif //FEATURE_DISPLAY

#ifdef GENERIC_CHARGRAB

        displayUpdate(send_buffer_array[0]);
        //displayUpdate(convert_cw_number_to_ascii(paddle_echo_buffer));
#endif

#if defined(DEBUG_SERVICE_SEND_BUFFER)
        debug_serial_port->print("service_send_buffer: send_char:");
        debug_serial_port->write(send_buffer_array[0]);
        debug_serial_port->println();
        Serial.flush();
#endif

        send_char(send_buffer_array[0], KEYER_NORMAL); //****************

        if (!pause_sending_buffer)
        {

#if defined(DEBUG_SERVICE_SEND_BUFFER)
          debug_serial_port->println("service_send_buffer: after send_char: remove_from_send_buffer");
          if (no_bytes_flag)
          {
            debug_serial_port->println("service_send_buffer: no_bytes_flag");
          }
          Serial.flush();
#endif

          if (!((send_buffer_array[0] > SERIAL_SEND_BUFFER_SPECIAL_START) && (send_buffer_array[0] < SERIAL_SEND_BUFFER_SPECIAL_END)))
          { // this is a friggin hack to fix something I can't explain with SO2R - Goody 20191217
            remove_from_send_buffer();

#if defined(DEBUG_SERVICE_SEND_BUFFER)
            debug_serial_port->println("service_send_buffer: after send_char: remove_from_send_buffer");
            if (no_bytes_flag)
            {
              debug_serial_port->println("service_send_buffer: no_bytes_flag");
            }
            Serial.flush();
#endif
          }
          else
          {

#if defined(DEBUG_SERVICE_SEND_BUFFER)
            debug_serial_port->println("service_send_buffer: snagged errant remove_from_send_buffer");
            Serial.flush();
#endif
          }
        }
      }
    } //if ((send_buffer_bytes) && (pause_sending_buffer == 0))
  }
  else
  { //if (send_buffer_status == SERIAL_SEND_BUFFER_NORMAL)

    if (send_buffer_status == SERIAL_SEND_BUFFER_TIMED_COMMAND)
    { // we're in a timed command

      if ((timed_command_in_progress == SERIAL_SEND_BUFFER_TIMED_KEY_DOWN) && (millis() > timed_command_end_time))
      {
        sending_mode = AUTOMATIC_SENDING;
        tx_and_sidetone_key(0);
        timed_command_in_progress = 0;
        send_buffer_status = SERIAL_SEND_BUFFER_NORMAL;

#if defined(DEBUG_SERVICE_SEND_BUFFER)
        debug_serial_port->println("service_send_buffer: SERIAL_SEND_BUFFER_TIMED_KEY_DOWN->SERIAL_SEND_BUFFER_NORMAL");
#endif
      }

      if ((timed_command_in_progress == SERIAL_SEND_BUFFER_TIMED_WAIT) && (millis() > timed_command_end_time))
      {
        timed_command_in_progress = 0;
        send_buffer_status = SERIAL_SEND_BUFFER_NORMAL;

#if defined(DEBUG_SERVICE_SEND_BUFFER)
        debug_serial_port->println("service_send_buffer: SERIAL_SEND_BUFFER_TIMED_WAIT->SERIAL_SEND_BUFFER_NORMAL");
#endif
      }
    }

    if (send_buffer_status == SERIAL_SEND_BUFFER_HOLD)
    { // we're in a send hold ; see if there's a SERIAL_SEND_BUFFER_HOLD_SEND_RELEASE in the buffer
      if (send_buffer_bytes == 0)
      {
        send_buffer_status = SERIAL_SEND_BUFFER_NORMAL; // this should never happen, but what the hell, we'll catch it here if it ever does happen
      }
      else
      {
        for (int z = 0; z < send_buffer_bytes; z++)
        {
          if (send_buffer_array[z] == SERIAL_SEND_BUFFER_HOLD_SEND_RELEASE)
          {
            send_buffer_status = SERIAL_SEND_BUFFER_NORMAL;
            z = send_buffer_bytes;
          }
        }
      }
    }

  } //if (send_buffer_status == SERIAL_SEND_BUFFER_NORMAL)

  //if the paddles are hit, dump the buffer
  check_paddles();
  if (((dit_buffer || dah_buffer) && (send_buffer_bytes)) && (keyer_machine_mode != BEACON))
  {

#if defined(DEBUG_SERVICE_SEND_BUFFER)
    debug_serial_port->println("service_send_buffer: buffer dump");
#endif

    clear_send_buffer();
    send_buffer_status = SERIAL_SEND_BUFFER_NORMAL;
    dit_buffer = 0;
    dah_buffer = 0;
#ifdef FEATURE_MEMORIES
    repeat_memory = 255;
#endif
#ifdef FEATURE_WINKEY_EMULATION
    if (winkey_sending && winkey_host_open)
    {
      winkey_port_write(0xc2 | winkey_sending | winkey_xoff, 0); // 0xc2 - BREAKIN bit set high
      winkey_interrupted = 1;
    }
#endif
  }
}

//-------------------------------------------------------------------------------------------------------
void clear_send_buffer()
{
#ifdef FEATURE_WINKEY_EMULATION
  winkey_xoff = 0;
#endif
  send_buffer_bytes = 0;
}

//-------------------------------------------------------------------------------------------------------
void remove_from_send_buffer()
{

#ifdef FEATURE_WINKEY_EMULATION
  if ((send_buffer_bytes < winkey_xon_threshold) && winkey_xoff && winkey_host_open)
  {
    winkey_xoff = 0;
    winkey_port_write(0xc0 | winkey_sending | winkey_xoff, 0); //send status /XOFF
  }
#endif

  if (send_buffer_bytes)
  {
    send_buffer_bytes--;
  }
  if (send_buffer_bytes)
  {
    for (int x = 0; x < send_buffer_bytes; x++)
    {
      send_buffer_array[x] = send_buffer_array[x + 1];
    }
#if defined(FEATURE_WINKEY_EMULATION) && defined(OPTION_WINKEY_FREQUENT_STATUS_REPORT)
    winkey_port_write(0xc0 | winkey_sending | winkey_xoff, 0);
#endif
  }
}

//-------------------------------------------------------------------------------------------------------

void add_to_send_buffer(byte incoming_serial_byte)
{

  if (send_buffer_bytes < send_buffer_size)
  {
    if (incoming_serial_byte != 127)
    {
      send_buffer_bytes++;
      send_buffer_array[send_buffer_bytes - 1] = incoming_serial_byte;

#ifdef FEATURE_WINKEY_EMULATION
      if ((send_buffer_bytes > winkey_xoff_threshold) && winkey_host_open)
      {
        winkey_xoff = 1;
        winkey_port_write(0xc0 | winkey_sending | winkey_xoff, 0); //send XOFF status
      }
#endif
    }
    else
    { // we got a backspace
      if (send_buffer_bytes)
      {
        send_buffer_bytes--;
      }
    }

#if defined(DEBUG_SERVICE_SEND_BUFFER)
    debug_serial_port->print("add_to_send_buffer: ");
    debug_serial_port->write(incoming_serial_byte);
    debug_serial_port->print(" [");
    debug_serial_port->print(incoming_serial_byte);
    debug_serial_port->println("]");
    debug_serial_port->print("send_buffer:");
    for (int x = 0; x < send_buffer_bytes; x++)
    {
      debug_serial_port->write(send_buffer_array[x]);
      debug_serial_port->print("[");
      debug_serial_port->print(send_buffer_array[x]);
      debug_serial_port->print("]");
    }
    debug_serial_port->println();
#endif
  }
  else
  {

#if defined(DEBUG_SERVICE_SEND_BUFFER)
    debug_serial_port->println("add_to_send_buffer: !send_buffer_bytes < send_buffer_size");
#endif
  }
}
//-------------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------------
#ifdef FEATURE_COMMAND_LINE_INTERFACE
void service_command_line_interface(PRIMARY_SERIAL_CLS *port_to_use)
{

  static byte cli_wait_for_cr_flag = 0;

  if (serial_backslash_command == 0)
  {
    incoming_serial_byte = uppercase(incoming_serial_byte);
    if ((incoming_serial_byte != 92) && (incoming_serial_byte != 27))
    { // we do not have a backslash or ESC

#if !defined(OPTION_EXCLUDE_MILL_MODE)
      if (configuration.cli_mode == CLI_NORMAL_MODE)
      {
        if (cli_prosign_flag)
        {
          add_to_send_buffer(SERIAL_SEND_BUFFER_PROSIGN);
          cli_prosign_flag = 0;
        }
        if (cli_wait_for_cr_to_send_cw)
        {
          if (cli_wait_for_cr_flag == 0)
          {
            if (incoming_serial_byte > 31)
            {
#ifdef DEBUG_CHECK_SERIAL
              port_to_use->println(F("check_serial: add_to_send_buffer(SERIAL_SEND_BUFFER_HOLD_SEND)"));
#endif
              add_to_send_buffer(SERIAL_SEND_BUFFER_HOLD_SEND);
              cli_wait_for_cr_flag = 1;
            }
          }
          else
          {
            if (incoming_serial_byte == 13)
            {
#ifdef DEBUG_CHECK_SERIAL
              port_to_use->println(F("check_serial: add_to_send_buffer(SERIAL_SEND_BUFFER_HOLD_SEND_RELEASE)"));
#endif
              add_to_send_buffer(SERIAL_SEND_BUFFER_HOLD_SEND_RELEASE);
              cli_wait_for_cr_flag = 0;
            }
          }
        }
        add_to_send_buffer(incoming_serial_byte);
      }
      else
      { // configuration.cli_mode != CLI_NORMAL_MODE
        if (configuration.cli_mode == CLI_MILL_MODE_KEYBOARD_RECEIVE)
        {
          port_to_use->write(incoming_serial_byte);
          if (incoming_serial_byte == 13)
          {
            port_to_use->println();
          }
#ifdef FEATURE_SD_CARD_SUPPORT
          sd_card_log("", incoming_serial_byte);
#endif
        }
        else
        { // configuration.cli_mode == CLI_MILL_MODE_PADDLE_SEND
          port_to_use->println();
          port_to_use->println();
          if (incoming_serial_byte == 13)
          {
            port_to_use->println();
          }
          port_to_use->write(incoming_serial_byte);
          configuration.cli_mode = CLI_MILL_MODE_KEYBOARD_RECEIVE;
#ifdef FEATURE_SD_CARD_SUPPORT
          sd_card_log("\r\nRX:", 0);
          sd_card_log("", incoming_serial_byte);
#endif
        }
      } //if (configuration.cli_mode == CLI_NORMAL_MODE)

#else //!defined(OPTION_EXCLUDE_MILL_MODE)

      if (cli_prosign_flag)
      {
        add_to_send_buffer(SERIAL_SEND_BUFFER_PROSIGN);
        cli_prosign_flag = 0;
      }
      if (cli_wait_for_cr_to_send_cw)
      {
        if (cli_wait_for_cr_flag == 0)
        {
          if (incoming_serial_byte > 31)
          {
#ifdef DEBUG_CHECK_SERIAL
            port_to_use->println(F("check_serial: add_to_send_buffer(SERIAL_SEND_BUFFER_HOLD_SEND)"));
#endif
            add_to_send_buffer(SERIAL_SEND_BUFFER_HOLD_SEND);
            cli_wait_for_cr_flag = 1;
          }
        }
        else
        {
          if (incoming_serial_byte == 13)
          {
#ifdef DEBUG_CHECK_SERIAL
            port_to_use->println(F("check_serial: add_to_send_buffer(SERIAL_SEND_BUFFER_HOLD_SEND_RELEASE)"));
#endif
            add_to_send_buffer(SERIAL_SEND_BUFFER_HOLD_SEND_RELEASE);
            cli_wait_for_cr_flag = 0;
          }
        }
      }
      add_to_send_buffer(incoming_serial_byte);

#endif // !defined(OPTION_EXCLUDE_MILL_MODE)

#ifdef FEATURE_MEMORIES
      if ((incoming_serial_byte != 13) && (incoming_serial_byte != 10))
      {
        repeat_memory = 255;
      }
#endif
    }
    else
    { //if ((incoming_serial_byte != 92) && (incoming_serial_byte != 27)) -- we got a backslash or ESC
      if (incoming_serial_byte == 92)
      { // backslash
        serial_backslash_command = 1;
        port_to_use->write(incoming_serial_byte);
      }
      else
      { // escape
        clear_send_buffer();
#ifdef FEATURE_MEMORIES
        play_memory_prempt = 1;
        repeat_memory = 255;
#endif
      }
    }
  }
  else
  { // (serial_backslash_command == 0) -- we already got a backslash
    incoming_serial_byte = uppercase(incoming_serial_byte);
    port_to_use->write(incoming_serial_byte);
    process_serial_command(port_to_use);
    serial_backslash_command = 0;
    port_to_use->println();
  }
}
#endif //FEATURE_COMMAND_LINE_INTERFACE

//-------------------------------------------------------------------------------------------------------

#if defined(FEATURE_SERIAL)
void check_serial()
{

#if defined(FEATURE_TRAINING_COMMAND_LINE_INTERFACE)
  if (check_serial_override)
  {
    return;
  }
#endif

#ifdef DEBUG_SERIAL_SEND_CW_CALLOUT
  byte debug_serial_send_cw[2];
  byte previous_tx = 0;
  byte previous_sidetone = 0;
#endif

#ifdef DEBUG_LOOP
  debug_serial_port->println(F("loop: entering check_serial"));
#endif

#ifdef FEATURE_WINKEY_EMULATION
  if (primary_serial_port_mode == SERIAL_WINKEY_EMULATION)
  {
    service_winkey(WINKEY_HOUSEKEEPING);
  }
#endif

  while (primary_serial_port->available() > 0)
  {
    incoming_serial_byte = primary_serial_port->read();
#ifdef FEATURE_SLEEP
    last_activity_time = millis();
#endif //FEATURE_SLEEP
#ifdef DEBUG_SERIAL_SEND_CW_CALLOUT
    debug_serial_send_cw[0] = (incoming_serial_byte & 0xf0) >> 4;
    debug_serial_send_cw[1] = incoming_serial_byte & 0x0f;
    for (byte x = 0; x < 2; x++)
    {
      if (debug_serial_send_cw[x] < 10)
      {
        debug_serial_send_cw[x] = debug_serial_send_cw[x] + 48;
      }
      else
      {
        debug_serial_send_cw[x] = debug_serial_send_cw[x] + 55;
      }
    }
    previous_tx = key_tx;
    key_tx = 0;
    previous_sidetone = configuration.sidetone_mode;
    configuration.sidetone_mode = SIDETONE_ON;
    send_char(debug_serial_send_cw[0], 0);
    send_char(debug_serial_send_cw[1], 0);
    key_tx = previous_tx;
    configuration.sidetone_mode = previous_sidetone;
#endif

#if !defined(FEATURE_WINKEY_EMULATION) && !defined(FEATURE_COMMAND_LINE_INTERFACE)
    primary_serial_port->println(F("No serial features enabled..."));
#endif

#if defined(FEATURE_WINKEY_EMULATION) && defined(FEATURE_COMMAND_LINE_INTERFACE)
    if (primary_serial_port_mode == SERIAL_WINKEY_EMULATION)
    {
      service_winkey(SERVICE_SERIAL_BYTE);
    }
    else
    {
      service_command_line_interface(primary_serial_port);
    }
#else //defined(FEATURE_WINKEY_EMULATION) && defined(FEATURE_COMMAND_LINE_INTERFACE)
#ifdef FEATURE_COMMAND_LINE_INTERFACE
    service_command_line_interface(primary_serial_port);
#endif //FEATURE_COMMAND_LINE_INTERFACE
#ifdef FEATURE_WINKEY_EMULATION
    service_winkey(SERVICE_SERIAL_BYTE);
#endif //FEATURE_WINKEY_EMULATION
#endif //defined(FEATURE_WINKEY_EMULATION) && defined(FEATURE_COMMAND_LINE_INTERFACE)

  } //while (primary_serial_port->available() > 0)

#ifdef FEATURE_COMMAND_LINE_INTERFACE_ON_SECONDARY_PORT
  while (secondary_serial_port->available() > 0)
  {
    incoming_serial_byte = secondary_serial_port->read();
#ifdef FEATURE_SLEEP
    last_activity_time = millis();
#endif //FEATURE_SLEEP
#ifdef DEBUG_SERIAL_SEND_CW_CALLOUT
    debug_serial_send_cw[0] = (incoming_serial_byte & 0xf0) >> 4;
    debug_serial_send_cw[1] = incoming_serial_byte & 0x0f;
    for (byte x = 0; x < 2; x++)
    {
      if (debug_serial_send_cw[x] < 10)
      {
        debug_serial_send_cw[x] = debug_serial_send_cw[x] + 48;
      }
      else
      {
        debug_serial_send_cw[x] = debug_serial_send_cw[x] + 55;
      }
    }
    previous_tx = key_tx;
    key_tx = 0;
    previous_sidetone = configuration.sidetone_mode;
    configuration.sidetone_mode = SIDETONE_ON;
    send_char(debug_serial_send_cw[0], 0);
    send_char(debug_serial_send_cw[1], 0);
    key_tx = previous_tx;
    configuration.sidetone_mode = previous_sidetone;
#endif //DEBUG_SERIAL_SEND_CW_CALLOUT
    service_command_line_interface(secondary_serial_port);
  }    //  while (secondary_serial_port->available() > 0)
#endif //FEATURE_COMMAND_LINE_INTERFACE_ON_SECONDARY_PORT
}
#endif //defined(FEATURE_SERIAL)

//---------------------------------------------------------------------

#if defined(FEATURE_SERIAL_HELP) && defined(FEATURE_SERIAL) && defined(FEATURE_COMMAND_LINE_INTERFACE)
void serial_page_pause(PRIMARY_SERIAL_CLS *port_to_use, byte seconds_timeout)
{

  unsigned long pause_start_time = millis();

  port_to_use->println("\r\nPress enter...");
  while ((!port_to_use->available()) && (((millis() - pause_start_time) / 1000) < seconds_timeout))
  {
  }
  while (port_to_use->available())
  {
    port_to_use->read();
  }
}
#endif //defined(FEATURE_SERIAL_HELP) && defined(FEATURE_SERIAL) && defined(FEATURE_COMMAND_LINE_INTERFACE)

//---------------------------------------------------------------------

#if defined(FEATURE_SERIAL_HELP) && defined(FEATURE_SERIAL) && defined(FEATURE_COMMAND_LINE_INTERFACE)
void print_serial_help(PRIMARY_SERIAL_CLS *port_to_use, byte paged_help)
{

  port_to_use->println(F("\n\rK3NG Keyer Help\n\r"));
  port_to_use->println(F("CLI commands:"));
  port_to_use->println(F("\\#\t\t: Play memory # x")); //Upper case to first letter only(WD9DMP)
  port_to_use->println(F("\\A\t\t: Iambic A"));
  port_to_use->println(F("\\B\t\t: Iambic B"));
  port_to_use->println(F("\\C\t\t: Single paddle")); //Upper case to first letter only(WD9DMP)
  port_to_use->println(F("\\D\t\t: Ultimatic"));
  port_to_use->println(F("\\E####\t\t: Set serial number to ####"));
  port_to_use->println(F("\\F####\t\t: Set sidetone to #### hz"));
  port_to_use->println(F("\\G\t\t: Switch to bug mode")); //Upper case to first letter only(WD9DMP)
#ifdef FEATURE_HELL
  port_to_use->println(F("\\H\t\t: Toggle CW / Hell mode"));
#endif
  port_to_use->println(F("\\I\t\t: TX line disable/enable"));
  port_to_use->println(F("\\J###\t\t: Set dah to dit ratio")); //Upper case to first letter only(WD9DMP)
#ifdef FEATURE_TRAINING_COMMAND_LINE_INTERFACE
  port_to_use->println(F("\\K\t\t: Training"));
#endif
  port_to_use->println(F("\\L##\t\t: Set weighting (50 = normal)"));
#ifdef FEATURE_FARNSWORTH
  port_to_use->println(F("\\M###\t\t: Set Farnsworth speed")); //Upper case to first letter only(WD9DMP)
#endif
  if (paged_help)
  {
    serial_page_pause(port_to_use, 10);
  }
  port_to_use->println(F("\\N\t\t: Toggle paddle reverse"));                  //Upper case to first letter only(WD9DMP)
  port_to_use->println(F("\\O\t\t: Toggle sidetone on/off"));                 //Added missing command (WD9DMP)
  port_to_use->println(F("\\Px<string>\t: Program memory #x with <string>")); //Upper case to first letter only(WD9DMP)
  port_to_use->println(F("\\Q#[#]\t\t: Switch to QRSS mode with ## second dit length"));
  port_to_use->println(F("\\R\t\t: Switch to regular speed (wpm) mode"));
  port_to_use->println(F("\\S\t\t: Status report")); //Upper case to first letter only(WD9DMP)
  port_to_use->println(F("\\T\t\t: Tune mode"));
  port_to_use->println(F("\\U\t\t: PTT toggle"));
#ifdef FEATURE_POTENTIOMETER
  port_to_use->println(F("\\V\t\t: Potentiometer activate/deactivate"));
#endif //FEATURE_POTENTIOMETER
  port_to_use->println(F("\\W#[#][#]\t: Change WPM to ###"));
  port_to_use->println(F("\\X#\t\t: Switch to transmitter #"));
  port_to_use->println(F("\\Y#\t\t: Change wordspace to #"));
#ifdef FEATURE_AUTOSPACE
  port_to_use->println(F("\\Z\t\t: Autospace on/off"));
#endif                                                                                                  //FEATURE_AUTOSPACE
  port_to_use->println(F("\\+\t\t: Create prosign"));                                                   //Changed description to match change log at top (WD9DMP)
  port_to_use->println(F("\\!##\t\t: Repeat play memory"));                                             //Added missing command(WD9DMP)
  port_to_use->println(F("\\|####\t\t: Set memory repeat (milliseconds)"));                             //Added missing command(WD9DMP)
  port_to_use->println(F("\\*\t\t: Toggle paddle echo"));                                               //Added missing command(WD9DMP)
  port_to_use->println(F("\\`\t\t: Toggle straight key echo"));                                         //Added missing command(WD9DMP)
  port_to_use->println(F("\\^\t\t: Toggle wait for carriage return to send CW / send CW immediately")); //Added missing command(WD9DMP)
#ifdef FEATURE_CMOS_SUPER_KEYER_IAMBIC_B_TIMING
  port_to_use->println(F("\\&\t\t: Toggle CMOS Super Keyer timing on/off")); //Upper case to first letter only(WD9DMP)
  port_to_use->println(F("\\%##\t\t: Set CMOS Super Keyer timing %"));       //Upper case to first letter only(WD9DMP)
#endif                                                                       //FEATURE_CMOS_SUPER_KEYER_IAMBIC_B_TIMING
  port_to_use->println(F("\\.\t\t: Toggle dit buffer on/off"));
  port_to_use->println(F("\\-\t\t: Toggle dah buffer on/off"));
  port_to_use->println(F("\\~\t\t: Reset unit"));                                                       //Added missing command(WD9DMP)
  port_to_use->println(F("\\:\t\t: Toggle cw send echo"));                                              //Added missing command(WD9DMP)
  port_to_use->println(F("\\{\t\t: QLF mode on/off"));                                                  //Added missing command(WD9DMP)
  port_to_use->println(F("\\>\t\t: Send serial number, then increment"));                               //Added missing command(WD9DMP)
  port_to_use->println(F("\\<\t\t: Send current serial number"));                                       //Added missing command(WD9DMP)
  port_to_use->println(F("\\(\t\t: Send current serial number in cut numbers"));                        //Added missing command(WD9DMP)
  port_to_use->println(F("\\)\t\t: Send serial number with cut numbers, then increment"));              //Added missing command(WD9DMP)
  port_to_use->println(F("\\[\t\t: Set quiet paddle interruption - 0 to 20 element lengths; 0 = off")); //Added missing command(WD9DMP)
#ifdef FEATURE_AMERICAN_MORSE
  port_to_use->println(F("\\=\t\t: Toggle American Morse mode")); //Added missing command(WD9DMP)
#endif
#ifdef FEATURE_POTENTIOMETER
  port_to_use->println(F("\\}####\t\t: Set Potentiometer range"));
#endif //FEATURE_POTENTIOMETER
#if !defined(OPTION_EXCLUDE_MILL_MODE)
  port_to_use->println(F("\\@\t\t: Mill Mode"));
#endif
  port_to_use->println(F("\\\\\t\t: Empty keyboard send buffer")); //Moved to end of command list (WD9DMP)
  if (paged_help)
  {
    serial_page_pause(port_to_use, 10);
  }
//Memory Macros below (WD9DMP)
#ifdef FEATURE_MEMORY_MACROS
  port_to_use->println(F("\nMemory Macros:"));
  port_to_use->println(F("\\#\t\t: Jump to memory #"));
  port_to_use->println(F("\\C\t\t: Send serial number with cut numbers, then increment")); //Added "then increment" (WD9DMP)
  port_to_use->println(F("\\D###\t\t: Delay for ### seconds"));
  port_to_use->println(F("\\E\t\t: Send serial number, then increment")); //Added "then increment" (WD9DMP)
  port_to_use->println(F("\\F####\t\t: Set sidetone to #### hz"));
#ifdef FEATURE_HELL
  port_to_use->println(F("\\H\t\t: Switch to Hell mode"));
#endif                                                 //FEATURE_HELL
  port_to_use->println(F("\\I\t\t: Insert memory #")); //Added missing macro (WD9DMP)
#ifdef FEATURE_HELL
  port_to_use->println(F("\\L\t\t: Switch to CW (from Hell mode)"));
#endif                                                                       //FEATURE_HELL
  port_to_use->println(F("\\N\t\t: Decrement serial number - do not send")); //Added "do not send" (WD9DMP)
  port_to_use->println(F("\\Q##\t\t: Switch to QRSS with ## second dit length"));
  port_to_use->println(F("\\R\t\t: Switch to regular speed mode"));
  port_to_use->println(F("\\S\t\t: Insert space")); //Added missing macro (WD9DMP)
  port_to_use->println(F("\\T###\t\t: Transmit for ### seconds"));
  port_to_use->println(F("\\U\t\t: Key PTT"));   //Upper case to first letter only(WD9DMP)
  port_to_use->println(F("\\V\t\t: Unkey PTT")); //Upper case to first letter only(WD9DMP)
  port_to_use->println(F("\\W###\t\t: Change WPM to ###"));
  port_to_use->println(F("\\X#\t\t: Switch to transmitter #"));
  port_to_use->println(F("\\Y#\t\t: Increase speed # WPM"));
  port_to_use->println(F("\\Z#\t\t: Decrease speed # WPM"));
  port_to_use->println(F("\\+\t\t: Prosign the next two characters")); //Added "the next two characters" (WD9DMP)
#endif                                                                 //FEATURE_MEMORY_MACROS

#if !defined(OPTION_EXCLUDE_EXTENDED_CLI_COMMANDS)
  if (paged_help)
  {
    serial_page_pause(port_to_use, 10);
  }
  port_to_use->println(F("\r\n\\:\tExtended CLLI commands"));
  port_to_use->println(F("\t\teepromdump\t\t- do a byte dump of EEPROM for troubleshooting"));
  port_to_use->println(F("\t\tsaveeeprom <filename>\t- store EEPROM in a file"));
  port_to_use->println(F("\t\tloadeeprom <filename> \t- load into EEPROM from a file"));
  port_to_use->println(F("\t\tprintlog\t\t- print the SD card log"));
  port_to_use->println(F("\t\tclearlog\t\t- clear the SD card log"));
  port_to_use->println(F("\t\tls <directory>\t\t- list files in SD card directory"));
  port_to_use->println(F("\t\tcat <filename>\t\t- print filename on SD card"));
  port_to_use->println(F("\t\tpl <transmitter> <mS>\t- Set PTT lead time"));
  port_to_use->println(F("\t\tpt <transmitter> <mS> \t- Set PTT tail time"));
#endif //OPTION_EXCLUDE_EXTENDED_CLI_COMMANDS

  port_to_use->println(F("\r\n\\/\t\t: Paginated help"));
}
#endif
//---------------------------------------------------------------------

#if defined(FEATURE_SERIAL) && defined(FEATURE_COMMAND_LINE_INTERFACE)
void process_serial_command(PRIMARY_SERIAL_CLS *port_to_use)
{

  int user_input_temp = 0;

#ifdef FEATURE_AMERICAN_MORSE
  static int previous_dah_to_dit_ratio = 300;
#endif //FEATURE_AMERICAN_MORSE

  //port_to_use->println();
  switch (incoming_serial_byte)
  {
  case '~':
#if defined(__AVR__)
    asm volatile("jmp 0"); /*wdt_enable(WDTO_30MS); while(1) {} ;*/
#else                      //__AVR__
    setup();
#endif                     //__AVR__
    break;                 // ~ - reset unit
  case '*':                // * - paddle echo on / off
    if (cli_paddle_echo)
    {
      cli_paddle_echo = 0;
    }
    else
    {
      cli_paddle_echo = 1;
    }
    break;
#if defined(FEATURE_STRAIGHT_KEY_ECHO)
  case '`':
    if (cli_straight_key_echo)
    {
      cli_straight_key_echo = 0;
    }
    else
    {
      cli_straight_key_echo = 1;
    }
    break;
#endif //FEATURE_STRAIGHT_KEY_ECHO
  case '+':
    cli_prosign_flag = 1;
    break;
#if defined(FEATURE_SERIAL_HELP)
  case '?':
    print_serial_help(port_to_use, 0);
    break; // ? = print help
  case '/':
    print_serial_help(port_to_use, 1);
    break;  // / = paged help
#endif      //FEATURE_SERIAL_HELP
  case 'A': // A - Iambic A mode
    configuration.keyer_mode = IAMBIC_A;
    configuration.dit_buffer_off = 0;
    configuration.dah_buffer_off = 0;
    config_dirty = 1;
    port_to_use->println(F("\r\nIambic A"));
    break;
  case 'B': // B - Iambic B mode
    configuration.keyer_mode = IAMBIC_B;
    configuration.dit_buffer_off = 0;
    configuration.dah_buffer_off = 0;
    config_dirty = 1;
    port_to_use->println(F("\r\nIambic B"));
    break;
  case 'C': // C - single paddle mode
    configuration.keyer_mode = SINGLE_PADDLE;
    config_dirty = 1;
    port_to_use->println(F("\r\nSingle Paddle"));
    break;
  case 'D': // D - Ultimatic mode
    configuration.keyer_mode = ULTIMATIC;
    configuration.dit_buffer_off = 1;
    configuration.dah_buffer_off = 1;
    config_dirty = 1;
    port_to_use->println(F("\r\nUltimatic"));
    break;
  case 'E':
    serial_set_serial_number(port_to_use);
    break; // E - set serial number
  case 'F':
    serial_set_sidetone_freq(port_to_use);
    break; // F - set sidetone frequency
  case 'G':
    configuration.keyer_mode = BUG;
    config_dirty = 1;
    port_to_use->println(F("\r\nBug"));
    break; // G - Bug mode
#ifdef FEATURE_HELL
  case 'H': // H - Hell mode
    if ((char_send_mode == CW) || (char_send_mode == AMERICAN_MORSE))
    {
      char_send_mode = HELL;
      port_to_use->println(F("\r\nHell mode"));
    }
    else
    {
      char_send_mode = CW;
      port_to_use->println(F("\r\nCW mode"));
    }
    break;
#endif //FEATURE_HELL
#ifdef FEATURE_AMERICAN_MORSE
  case '=': // = - American Morse
    if ((char_send_mode == CW) || (char_send_mode == HELL))
    {
      char_send_mode = AMERICAN_MORSE;
      port_to_use->println(F("\r\nAmerican Morse mode"));
      previous_dah_to_dit_ratio = configuration.dah_to_dit_ratio;
      configuration.dah_to_dit_ratio = 200;
    }
    else
    {
      char_send_mode = CW;
      port_to_use->println(F("\r\nInternational CW mode"));
      configuration.dah_to_dit_ratio = previous_dah_to_dit_ratio;
    }
    break;
#endif //FEATURE_AMERICAN_MORSE

  case 'I': // I - transmit line on/off
    //port_to_use->print(F("\r\nTX o")); //WD9DMP-1
    if (key_tx && keyer_machine_mode != KEYER_COMMAND_MODE)
    { //Added check that keyer is NOT in command mode or keyer might be enabled for paddle commands (WD9DMP-1)
      key_tx = 0;
      port_to_use->println(F("\r\nTX off")); //WD9DMP-1
    }
    else if (!key_tx && keyer_machine_mode != KEYER_COMMAND_MODE)
    { //Added check that keyer is NOT in command mode or keyer might be enabled for paddle commands (WD9DMP-1)
      key_tx = 1;
      port_to_use->println(F("\r\nTX on")); //WD9DMP-1
    }
    else
    {
      port_to_use->print(F("\r\nERROR: Keyer in Command Mode\r\n"));
    } //Print error message if keyer in Command Mode and user tries to change tx line(s) on/off. (WD9DMP-1)
    break;
#ifdef FEATURE_MEMORIES
  case 33:
    repeat_play_memory(port_to_use);
    break; // ! - repeat play
  case 124:
    serial_set_memory_repeat(port_to_use);
    break; // | - set memory repeat time
  case 48:
    serial_play_memory(9);
    break; // 0 - play memory 10
  case 49: // 1-9 - play memory #
  case 50:
  case 51:
  case 52:
  case 53:
  case 54:
  case 55:
  case 56:
  case 57:
    serial_play_memory(incoming_serial_byte - 49);
    break;
  case 80:
    repeat_memory = 255;
    serial_program_memory(port_to_use);
    break; // P - program memory
#endif     //FEATURE_MEMORIES
  case 'Q':
    serial_qrss_mode();
    break; // Q - activate QRSS mode
  case 'R':
    speed_mode = SPEED_NORMAL;
    port_to_use->println(F("\r\nQRSS Off"));
    break; // R - activate regular timing mode
  case 'S':
    serial_status(port_to_use);
    break; // S - Status command
  case 'J':
    serial_set_dit_to_dah_ratio(port_to_use);
    break; // J - dit to dah ratio
#ifdef FEATURE_TRAINING_COMMAND_LINE_INTERFACE
  case 'K':
    serial_cw_practice(port_to_use);
    break; // K - CW practice
#endif     //FEATURE_TRAINING_COMMAND_LINE_INTERFACE
  case 'L':
    serial_set_weighting(port_to_use);
    break;
#ifdef FEATURE_FARNSWORTH
  case 'M':
    serial_set_farnsworth(port_to_use);
    break; // M - set Farnsworth speed
#endif
  case 'N': // N - paddle reverse
    port_to_use->print(F("\r\nPaddles "));
    if (configuration.paddle_mode == PADDLE_NORMAL)
    {
      configuration.paddle_mode = PADDLE_REVERSE;
      port_to_use->println(F("reversed"));
    }
    else
    {
      configuration.paddle_mode = PADDLE_NORMAL;
      port_to_use->println(F("normal"));
    }
    config_dirty = 1;
    break;
  case 'O': // O - cycle through sidetone modes on, paddle only and off - enhanced by Marc-Andre, VE2EVN
    port_to_use->print(F("\r\nSidetone "));
    if (configuration.sidetone_mode == SIDETONE_PADDLE_ONLY)
    {
      configuration.sidetone_mode = SIDETONE_OFF;
      boop();
      port_to_use->println(F("OFF"));
    }
    else if (configuration.sidetone_mode == SIDETONE_ON)
    {
      configuration.sidetone_mode = SIDETONE_PADDLE_ONLY;
      beep();
      delay(200);
      beep();
      port_to_use->println(F("PADDLE ONLY"));
    }
    else
    {
      configuration.sidetone_mode = SIDETONE_ON;
      beep();
      port_to_use->println(F("ON"));
    }
    config_dirty = 1;
    break;

  case 'T': // T - tune
#ifdef FEATURE_MEMORIES
    repeat_memory = 255;
#endif
    serial_tune_command(port_to_use);
    break;
  case 'U':
    port_to_use->print(F("\r\nPTT o"));
    if (ptt_line_activated)
    {
      manual_ptt_invoke = 0;
      ptt_unkey();
      port_to_use->println(F("ff"));
    }
    else
    {
      manual_ptt_invoke = 1;
      ptt_key();
      port_to_use->println(F("n"));
    }
    break;
#ifdef FEATURE_POTENTIOMETER
  case 'V': // V - toggle pot activation
    port_to_use->print(F("\r\nPotentiometer "));
    configuration.pot_activated = !configuration.pot_activated;
    if (configuration.pot_activated)
    {
      port_to_use->print(F("A"));
    }
    else
    {
      port_to_use->print(F("Dea"));
    }
    port_to_use->println(F("ctivated"));
    config_dirty = 1;
    break;
  case '}':
    serial_set_pot_low_high(port_to_use);
    break;
#endif
  case 'W':
    serial_wpm_set(port_to_use);
    break; // W - set WPM
  case 'X':
    serial_switch_tx(port_to_use);
    break; // X - switch transmitter
  case 'Y':
    serial_change_wordspace(port_to_use);
    break;
#ifdef FEATURE_AUTOSPACE
  case 'Z':
    port_to_use->print(F("\r\nAutospace O"));
    if (configuration.autospace_active)
    {
      configuration.autospace_active = 0;
      config_dirty = 1;
      port_to_use->println(F("ff"));
    }
    else
    {
      configuration.autospace_active = 1;
      config_dirty = 1;
      port_to_use->println(F("n"));
    }
    break;
#endif

  case 92: // \ - double backslash - clear serial send buffer
    clear_send_buffer();
#if !defined(OPTION_DISABLE_SERIAL_PORT_CHECKING_WHILE_SENDING_CW)
    loop_element_lengths_breakout_flag = 0;
#endif
#ifdef FEATURE_MEMORIES
    play_memory_prempt = 1;
    repeat_memory = 255;
#endif
    break;

  case '^': // ^ - toggle send CW send immediately
    if (cli_wait_for_cr_to_send_cw)
    {
      cli_wait_for_cr_to_send_cw = 0;
      port_to_use->println(F("\r\nSend CW immediately"));
    }
    else
    {
      cli_wait_for_cr_to_send_cw = 1;
      port_to_use->println(F("\r\nWait for CR to send CW"));
    }
    break;
#ifdef FEATURE_CMOS_SUPER_KEYER_IAMBIC_B_TIMING
  case '&':
    port_to_use->print(F("\r\nCMOS Super Keyer Timing O"));
    if (configuration.cmos_super_keyer_iambic_b_timing_on)
    {
      configuration.cmos_super_keyer_iambic_b_timing_on = 0;
      port_to_use->println(F("ff"));
    }
    else
    {
      configuration.cmos_super_keyer_iambic_b_timing_on = 1;
      port_to_use->println(F("n"));
      configuration.keyer_mode = IAMBIC_B;
    }
    config_dirty = 1;
    break;
  case '%':
    user_input_temp = serial_get_number_input(2, -1, 100, port_to_use, RAISE_ERROR_MSG);
    if ((user_input_temp >= 0) && (user_input_temp < 100))
    {
      configuration.cmos_super_keyer_iambic_b_timing_percent = user_input_temp;
      port_to_use->println(F("\r\nCMOS Super Keyer Timing Set."));
    }
    config_dirty = 1;
    break;
#endif //FEATURE_CMOS_SUPER_KEYER_IAMBIC_B_TIMING
  case '.':
    port_to_use->print(F("\r\nDit Buffer O"));
    if (configuration.dit_buffer_off)
    {
      configuration.dit_buffer_off = 0;
      port_to_use->println(F("n"));
    }
    else
    {
      configuration.dit_buffer_off = 1;
      port_to_use->println(F("ff"));
    }
    config_dirty = 1;
    break;
  case '-':
    port_to_use->print(F("\r\nDah Buffer O"));
    if (configuration.dah_buffer_off)
    {
      configuration.dah_buffer_off = 0;
      port_to_use->println(F("n"));
    }
    else
    {
      configuration.dah_buffer_off = 1;
      port_to_use->println(F("ff"));
    }
    config_dirty = 1;
    break;
  case ';':
    if (cw_send_echo_inhibit)
      cw_send_echo_inhibit = 0;
    else
      cw_send_echo_inhibit = 1;
    break;
#ifdef FEATURE_QLF
  case '{':
    port_to_use->print(F("\r\nQLF: O"));
    if (qlf_active)
    {
      qlf_active = 0;
      port_to_use->println(F("ff"));
    }
    else
    {
      qlf_active = 1;
      port_to_use->println(F("n"));
    }
    break;
#endif //FEATURE_QLF

  case '>':
    send_serial_number(0, 1, 1);
    break;
  case '<':
    send_serial_number(0, 0, 1);
    break;
  case '(':
    send_serial_number(1, 0, 1);
    break;
  case ')':
    send_serial_number(1, 1, 1);
    break;
  case '[':
    user_input_temp = serial_get_number_input(2, -1, 21, port_to_use, RAISE_ERROR_MSG);
    if ((user_input_temp >= 0) && (user_input_temp < 21))
    {
      configuration.paddle_interruption_quiet_time_element_lengths = user_input_temp;
      port_to_use->println(F("\r\nPaddle Interruption Quiet Time set."));
    }
    config_dirty = 1;
    break;
#if !defined(OPTION_EXCLUDE_MILL_MODE)
  case '@':
    switch (configuration.cli_mode)
    {
    case CLI_NORMAL_MODE:
      configuration.cli_mode = CLI_MILL_MODE_KEYBOARD_RECEIVE;
      port_to_use->println(F("\r\nMill Mode On"));
      break;
    case CLI_MILL_MODE_PADDLE_SEND:
    case CLI_MILL_MODE_KEYBOARD_RECEIVE:
      configuration.cli_mode = CLI_NORMAL_MODE;
      port_to_use->println(F("\r\nMill Mode Off"));
      break;
    }
    config_dirty = 1;
    break;
#endif // !defined(OPTION_EXCLUDE_MILL_MODE)
  case '"':
    port_to_use->print(F("\r\nPTT buffered character hold active O"));
    if (configuration.ptt_buffer_hold_active)
    {
      configuration.ptt_buffer_hold_active = 0;
      port_to_use->println(F("ff"));
    }
    else
    {
      configuration.ptt_buffer_hold_active = 1;
      port_to_use->println(F("n"));
    }
    config_dirty = 1;
    break;
#if !defined(OPTION_EXCLUDE_EXTENDED_CLI_COMMANDS)
  case ':':
    cli_extended_commands(port_to_use);
    break;
#endif

  default:
    port_to_use->println(F("\r\nUnknown command"));
    break;
  }
}
#endif //defined(FEATURE_SERIAL) && defined(FEATURE_COMMAND_LINE_INTERFACE)

#ifdef FEATURE_PADDLE_ECHO
void service_paddle_echo()
{

#ifdef DEBUG_LOOP
  debug_serial_port->println(F("loop: entering service_paddle_echo"));
#endif

  static byte paddle_echo_space_sent = 1;
  byte character_to_send = 0;
  static byte no_space = 0;

#if defined(OPTION_PROSIGN_SUPPORT)
  byte byte_temp = 0;
  static char *prosign_temp = (char *)"";
#endif

#if defined(FEATURE_DISPLAY) && defined(OPTION_DISPLAY_NON_ENGLISH_EXTENSIONS)
  byte ascii_temp = 0;
#endif //defined(FEATURE_DISPLAY) && defined(OPTION_DISPLAY_NON_ENGLISH_EXTENSIONS)

#if defined(FEATURE_CW_COMPUTER_KEYBOARD)
  static byte backspace_flag = 0;
  if (paddle_echo_buffer == 111111)
  {
    paddle_echo_buffer_decode_time = 0;
    backspace_flag = 1;
  }    //this is a special hack to make repeating backspace work
#endif //defined(FEATURE_CW_COMPUTER_KEYBOARD)

#ifdef FEATURE_SD_CARD_SUPPORT
  char temp_string[2];
#endif

  if ((paddle_echo_buffer) && (millis() > paddle_echo_buffer_decode_time))
  {

#ifdef FEATURE_DISPLAY
    if (lcd_paddle_echo)
    {
#if defined(OPTION_PROSIGN_SUPPORT)
#ifndef OPTION_DISPLAY_NON_ENGLISH_EXTENSIONS
      byte_temp = convert_cw_number_to_ascii(paddle_echo_buffer);
      if ((byte_temp > PROSIGN_START) && (byte_temp < PROSIGN_END))
      {
        prosign_temp = convert_prosign(byte_temp);
        display_scroll_print_char(prosign_temp[0]);
        display_scroll_print_char(prosign_temp[1]);
      }
      else
      {
        display_scroll_print_char(byte(convert_cw_number_to_ascii(paddle_echo_buffer)));
      }
#else  //OPTION_DISPLAY_NON_ENGLISH_EXTENSIONS
      ascii_temp = byte(convert_cw_number_to_ascii(paddle_echo_buffer));
      if ((ascii_temp > PROSIGN_START) && (ascii_temp < PROSIGN_END))
      {
        prosign_temp = convert_prosign(ascii_temp);
        display_scroll_print_char(prosign_temp[0]);
        display_scroll_print_char(prosign_temp[1]);
      }
      else
      {
        switch (ascii_temp)
        {
        case 220:
          ascii_temp = 0;
          break; // U_umlaut  (D, ...)
        case 214:
          ascii_temp = 1;
          break; // O_umlaut  (D, SM, OH, ...)
        case 196:
          ascii_temp = 2;
          break; // A_umlaut  (D, SM, OH, ...)
        case 198:
          ascii_temp = 3;
          break; // AE_capital (OZ, LA)
        case 216:
          ascii_temp = 4;
          break; // OE_capital (OZ, LA)
        case 197:
          ascii_temp = 6;
          break; // AA_capital (OZ, LA, SM)
        case 209:
          ascii_temp = 7;
          break; // N-tilde (EA)
        }
        display_scroll_print_char(ascii_temp);
      }
#endif //OPTION_DISPLAY_NON_ENGLISH_EXTENSIONS

#else // ! OPTION_PROSIGN_SUPPORT

#ifndef OPTION_DISPLAY_NON_ENGLISH_EXTENSIONS
      display_scroll_print_char(byte(convert_cw_number_to_ascii(paddle_echo_buffer)));
#else  //OPTION_DISPLAY_NON_ENGLISH_EXTENSIONS
      ascii_temp = byte(convert_cw_number_to_ascii(paddle_echo_buffer));
      switch (ascii_temp)
      {
      case 220:
        ascii_temp = 0;
        break; // U_umlaut  (D, ...)
      case 214:
        ascii_temp = 1;
        break; // O_umlaut  (D, SM, OH, ...)
      case 196:
        ascii_temp = 2;
        break; // A_umlaut  (D, SM, OH, ...)
      case 198:
        ascii_temp = 3;
        break; // AE_capital (OZ, LA)
      case 216:
        ascii_temp = 4;
        break; // OE_capital (OZ, LA)
      case 197:
        ascii_temp = 6;
        break; // AA_capital (OZ, LA, SM)
      case 209:
        ascii_temp = 7;
        break; // N-tilde (EA)
      }
      display_scroll_print_char(ascii_temp);
#endif //OPTION_DISPLAY_NON_ENGLISH_EXTENSIONS
#endif //OPTION_PROSIGN_SUPPORT
    }
#endif //FEATURE_DISPLAY

#ifdef GENERIC_CHARGRAB

    displayUpdate(convert_cw_number_to_ascii(paddle_echo_buffer));
#endif

#if defined(FEATURE_SERIAL) && defined(FEATURE_COMMAND_LINE_INTERFACE)
#if defined(OPTION_PROSIGN_SUPPORT)
    byte_temp = convert_cw_number_to_ascii(paddle_echo_buffer);
    if (cli_paddle_echo)
    {
      if ((byte_temp > PROSIGN_START) && (byte_temp < PROSIGN_END))
      {
        primary_serial_port->print(prosign_temp[0]);
        primary_serial_port->print(prosign_temp[1]);
#ifdef FEATURE_COMMAND_LINE_INTERFACE_ON_SECONDARY_PORT
        secondary_serial_port->print(prosign_temp[0]);
        secondary_serial_port->print(prosign_temp[1]);
#endif //FEATURE_COMMAND_LINE_INTERFACE_ON_SECONDARY_PORT
      }
      else
      {
        if (configuration.cli_mode == CLI_MILL_MODE_KEYBOARD_RECEIVE)
        {
          primary_serial_port->println();
          primary_serial_port->println();
#ifdef FEATURE_COMMAND_LINE_INTERFACE_ON_SECONDARY_PORT
          secondary_serial_port->println();
          secondary_serial_port->println();
#endif
#ifdef FEATURE_SD_CARD_SUPPORT
          sd_card_log("\r\nTX:", 0);
#endif
          configuration.cli_mode = CLI_MILL_MODE_PADDLE_SEND;
        }
        primary_serial_port->write(byte_temp);
#ifdef FEATURE_COMMAND_LINE_INTERFACE_ON_SECONDARY_PORT
        secondary_serial_port->write(byte_temp);
#endif //FEATURE_COMMAND_LINE_INTERFACE_ON_SECONDARY_PORT
#ifdef FEATURE_SD_CARD_SUPPORT
        sd_card_log("", incoming_serial_byte);
#endif
      }
    }

#else // ! OPTION_PROSIGN_SUPPORT
    if (cli_paddle_echo)
    {
      if (configuration.cli_mode == CLI_MILL_MODE_KEYBOARD_RECEIVE)
      {
        primary_serial_port->println();
        primary_serial_port->println();
#ifdef FEATURE_COMMAND_LINE_INTERFACE_ON_SECONDARY_PORT
        secondary_serial_port->println();
        secondary_serial_port->println();
#endif
#ifdef FEATURE_SD_CARD_SUPPORT
        sd_card_log("\r\nTX:", 0);
#endif
        configuration.cli_mode = CLI_MILL_MODE_PADDLE_SEND;
      }

      primary_serial_port->write(byte(convert_cw_number_to_ascii(paddle_echo_buffer)));

#ifdef FEATURE_COMMAND_LINE_INTERFACE_ON_SECONDARY_PORT
      secondary_serial_port->write(byte(convert_cw_number_to_ascii(paddle_echo_buffer)));
#endif
#ifdef FEATURE_SD_CARD_SUPPORT
      sd_card_log("", convert_cw_number_to_ascii(paddle_echo_buffer));
#endif
    }
#endif //OPTION_PROSIGN_SUPPORT
#endif //defined(FEATURE_SERIAL) && defined(FEATURE_COMMAND_LINE_INTERFACE)

    paddle_echo_buffer = 0;
    paddle_echo_buffer_decode_time = millis() + (float(600 / configuration.wpm) * length_letterspace);
    paddle_echo_space_sent = 0;
  }

  // is it time to echo a space?
  if ((paddle_echo_buffer == 0) && (millis() > (paddle_echo_buffer_decode_time + (float(1200 / configuration.wpm) * (configuration.length_wordspace - length_letterspace)))) && (!paddle_echo_space_sent))
  {

#if defined(FEATURE_CW_COMPUTER_KEYBOARD)
    if (!no_space)
    {
      Keyboard.write(' ');
#ifdef DEBUG_CW_COMPUTER_KEYBOARD
      debug_serial_port->println("service_paddle_echo: Keyboard.write: <space>");
#endif //DEBUG_CW_COMPUTER_KEYBOARD
    }
    no_space = 0;
#endif //defined(FEATURE_CW_COMPUTER_KEYBOARD)

#ifdef FEATURE_DISPLAY
    if (lcd_paddle_echo)
    {
      display_scroll_print_char(' ');
    }
#endif //FEATURE_DISPLAY

#ifdef GENERIC_CHARGRAB
    displayUpdate(' ');
#endif

#if defined(FEATURE_SERIAL) && defined(FEATURE_COMMAND_LINE_INTERFACE)
    if (cli_paddle_echo)
    {

      primary_serial_port->write(" ");

#ifdef FEATURE_COMMAND_LINE_INTERFACE_ON_SECONDARY_PORT
      secondary_serial_port->write(" ");
#endif
    }
#endif //defined(FEATURE_SERIAL) && defined(FEATURE_COMMAND_LINE_INTERFACE)

#ifdef FEATURE_SD_CARD_SUPPORT
    sd_card_log(" ", 0);
#endif

    paddle_echo_space_sent = 1;
  }
}
#endif //FEATURE_PADDLE_ECHO

//---------------------------------------------------------------------

#if defined(FEATURE_SERIAL) && defined(FEATURE_COMMAND_LINE_INTERFACE)
int serial_get_number_input(byte places, int lower_limit, int upper_limit, PRIMARY_SERIAL_CLS *port_to_use, int raise_error_message)
{
  byte incoming_serial_byte = 0;
  byte looping = 1;
  byte error = 0;
  String numberstring = "";
  byte numberindex = 0;
  int numbers[6];

  while (looping)
  {
    if (port_to_use->available() == 0)
    { // wait for the next keystroke
      if (keyer_machine_mode == KEYER_NORMAL)
      { // might as well do something while we're waiting
        check_paddles();
        service_dit_dah_buffers();
        service_send_buffer(PRINTCHAR);

        check_ptt_tail();
#ifdef FEATURE_POTENTIOMETER
        if (configuration.pot_activated)
        {
          check_potentiometer();
        }
#endif

#ifdef FEATURE_ROTARY_ENCODER
        check_rotary_encoder();
#endif //FEATURE_ROTARY_ENCODER
      }
    }
    else
    {
      incoming_serial_byte = port_to_use->read();
      port_to_use->write(incoming_serial_byte);
      if ((incoming_serial_byte > 47) && (incoming_serial_byte < 58))
      { // ascii 48-57 = "0" - "9")
        numberstring = numberstring + incoming_serial_byte;
        numbers[numberindex] = incoming_serial_byte;
        numberindex++;
        if (numberindex > places)
        {
          looping = 0;
          error = 1;
        }
      }
      else
      {
        if (incoming_serial_byte == 13)
        { // carriage return - get out
          looping = 0;
        }
        else
        { // bogus input - error out
          looping = 0;
          error = 1;
        }
      }
    }
  }
  if (error)
  {
    if (raise_error_message == RAISE_ERROR_MSG)
    {
      port_to_use->println(F("Error..."));
    }
    while (port_to_use->available() > 0)
    {
      incoming_serial_byte = port_to_use->read();
    } // clear out buffer
    return (-1);
  }
  else
  {
    int y = 1;
    int return_number = 0;
    for (int x = (numberindex - 1); x >= 0; x = x - 1)
    {
      return_number = return_number + ((numbers[x] - 48) * y);
      y = y * 10;
    }
    if ((return_number > lower_limit) && (return_number < upper_limit))
    {
      return (return_number);
    }
    else
    {
      if (raise_error_message == RAISE_ERROR_MSG)
      {
        port_to_use->println(F("Error..."));
      }
      return (-1);
    }
  }
}
#endif

//---------------------------------------------------------------------

#if defined(FEATURE_SERIAL) && defined(FEATURE_COMMAND_LINE_INTERFACE)
void serial_change_wordspace(PRIMARY_SERIAL_CLS *port_to_use)
{
  int set_wordspace_to = serial_get_number_input(2, 0, 100, port_to_use, RAISE_ERROR_MSG);
  if (set_wordspace_to > 0)
  {
    config_dirty = 1;
    configuration.length_wordspace = set_wordspace_to;
    port_to_use->write("\r\nWordspace set to ");
    port_to_use->println(set_wordspace_to, DEC);
  }
}
#endif

//---------------------------------------------------------------------

#if defined(FEATURE_SERIAL) && defined(FEATURE_COMMAND_LINE_INTERFACE)
void serial_switch_tx(PRIMARY_SERIAL_CLS *port_to_use)
{
  int set_tx_to = serial_get_number_input(1, 0, 7, port_to_use, RAISE_ERROR_MSG);
  if (set_tx_to > 0)
  {
    switch (set_tx_to)
    {
    case 1:
      switch_to_tx_silent(1);
      port_to_use->print(F("\r\nSwitching to TX #"));
      port_to_use->println(F("1"));
      break;
    case 2:
      if ((ptt_tx_2) || (tx_key_line_2))
      {
        switch_to_tx_silent(2);
        port_to_use->print(F("\r\nSwitching to TX #"));
      }
      port_to_use->println(F("2"));
      break;
    case 3:
      if ((ptt_tx_3) || (tx_key_line_3))
      {
        switch_to_tx_silent(3);
        port_to_use->print(F("\r\nSwitching to TX #"));
      }
      port_to_use->println(F("3"));
      break;
    case 4:
      if ((ptt_tx_4) || (tx_key_line_4))
      {
        switch_to_tx_silent(4);
        port_to_use->print(F("\r\nSwitching to TX #"));
      }
      port_to_use->println(F("4"));
      break;
    case 5:
      if ((ptt_tx_5) || (tx_key_line_5))
      {
        switch_to_tx_silent(5);
        port_to_use->print(F("\r\nSwitching to TX #"));
      }
      port_to_use->println(F("5"));
      break;
    case 6:
      if ((ptt_tx_6) || (tx_key_line_6))
      {
        switch_to_tx_silent(6);
        port_to_use->print(F("\r\nSwitching to TX #"));
      }
      port_to_use->println(F("6"));
      break;
    }
  }
}
#endif

//---------------------------------------------------------------------
#if defined(FEATURE_SERIAL) && defined(FEATURE_COMMAND_LINE_INTERFACE)
void serial_set_dit_to_dah_ratio(PRIMARY_SERIAL_CLS *port_to_use)
{
  int set_ratio_to = serial_get_number_input(4, 99, 1000, port_to_use, DONT_RAISE_ERROR_MSG);
  // if ((set_ratio_to > 99) && (set_ratio_to < 1000)) {
  //   configuration.dah_to_dit_ratio = set_ratio_to;
  //   port_to_use->print(F("Setting dah to dit ratio to "));
  //   port_to_use->println((float(configuration.dah_to_dit_ratio)/100));
  //   config_dirty = 1;
  // }

  if ((set_ratio_to < 100) || (set_ratio_to > 999))
  {
    set_ratio_to = 300;
  }
  configuration.dah_to_dit_ratio = set_ratio_to;
  port_to_use->write("\r\nDah to dit ratio set to ");
  port_to_use->println((float(configuration.dah_to_dit_ratio) / 100));
  config_dirty = 1;
}
#endif

//---------------------------------------------------------------------

#if defined(FEATURE_SERIAL) && defined(FEATURE_COMMAND_LINE_INTERFACE)
void serial_set_serial_number(PRIMARY_SERIAL_CLS *port_to_use)
{
  int set_serial_number_to = serial_get_number_input(4, 0, 10000, port_to_use, RAISE_ERROR_MSG);
  if (set_serial_number_to > 0)
  {
    serial_number = set_serial_number_to;
    port_to_use->write("\r\nSetting serial number to ");
    port_to_use->println(serial_number);
  }
}
#endif

//---------------------------------------------------------------------

#if defined(FEATURE_SERIAL) && defined(FEATURE_COMMAND_LINE_INTERFACE) && defined(FEATURE_POTENTIOMETER)
void serial_set_pot_low_high(PRIMARY_SERIAL_CLS *port_to_use)
{
  int serial_get_number = serial_get_number_input(4, 500, 10000, port_to_use, RAISE_ERROR_MSG);
  int low_number = (serial_get_number / 100);
  int high_number = serial_get_number % (int(serial_get_number / 100) * 100);
  if ((low_number < high_number) && (low_number >= wpm_limit_low) && (high_number <= wpm_limit_high))
  {
    port_to_use->print(F("\r\nSetting potentiometer range to "));
    port_to_use->print(low_number);
    port_to_use->print(F(" - "));
    port_to_use->print(high_number);
    port_to_use->println(F(" WPM"));
    pot_wpm_low_value = low_number;
    pot_wpm_high_value = high_number;
  }
  else
  {
    port_to_use->println(F("\nError"));
  }
}
#endif

//---------------------------------------------------------------------

#if defined(FEATURE_SERIAL) && defined(FEATURE_COMMAND_LINE_INTERFACE)
void serial_set_sidetone_freq(PRIMARY_SERIAL_CLS *port_to_use)
{
  int set_sidetone_hz = serial_get_number_input(4, (sidetone_hz_limit_low - 1), (sidetone_hz_limit_high + 1), port_to_use, RAISE_ERROR_MSG);
  if ((set_sidetone_hz > sidetone_hz_limit_low) && (set_sidetone_hz < sidetone_hz_limit_high))
  {
    port_to_use->write("\r\nSetting sidetone to ");
    port_to_use->print(set_sidetone_hz, DEC);
    port_to_use->println(F(" hz"));
    configuration.hz_sidetone = set_sidetone_hz;
    config_dirty = 1;
#ifdef FEATURE_DISPLAY
    if (LCD_COLUMNS < 9)
      lcd_center_print_timed(String(configuration.hz_sidetone) + " Hz", 0, default_display_msg_delay);
    else
      lcd_center_print_timed("Sidetone " + String(configuration.hz_sidetone) + " Hz", 0, default_display_msg_delay);
#endif // FEATURE_DISPLAY
  }
}
#endif

//---------------------------------------------------------------------

#if defined(FEATURE_SERIAL) && defined(FEATURE_COMMAND_LINE_INTERFACE)
void serial_wpm_set(PRIMARY_SERIAL_CLS *port_to_use)
{
  int set_wpm = serial_get_number_input(3, 0, 1000, port_to_use, RAISE_ERROR_MSG);
  if (set_wpm > 0)
  {
    speed_set(set_wpm);
    port_to_use->write("\r\nSetting WPM to ");
    port_to_use->println(set_wpm, DEC);
    config_dirty = 1;
  }
}
#endif

//---------------------------------------------------------------------

//---------------------------------------------------------------------

#if defined(FEATURE_SERIAL) && defined(FEATURE_COMMAND_LINE_INTERFACE)
void serial_set_weighting(PRIMARY_SERIAL_CLS *port_to_use)
{
  int set_weighting = serial_get_number_input(2, 9, 91, port_to_use, DONT_RAISE_ERROR_MSG);
  if (set_weighting < 1)
  {
    set_weighting = 50;
  }
  configuration.weighting = set_weighting;
  port_to_use->write("\r\nSetting weighting to ");
  port_to_use->println(set_weighting, DEC);
}
#endif

//---------------------------------------------------------------------

#if defined(FEATURE_SERIAL) && defined(FEATURE_COMMAND_LINE_INTERFACE)
void serial_tune_command(PRIMARY_SERIAL_CLS *port_to_use)
{
  byte incoming;

  delay(100);
  while (port_to_use->available() > 0)
  { // clear out the buffer if anything is there
    incoming = port_to_use->read();
  }

  sending_mode = MANUAL_SENDING;
  tx_and_sidetone_key(1);
  port_to_use->println(F("\r\nKeying tx - press a key to unkey"));
#ifdef FEATURE_COMMAND_BUTTONS
  while ((port_to_use->available() == 0) && (!analogbuttonread(0)))
  {
  } // keystroke or button0 hit gets us out of here
#else
  while (port_to_use->available() == 0)
  {
  }
#endif
  while (port_to_use->available() > 0)
  { // clear out the buffer if anything is there
    incoming = port_to_use->read();
  }
  tx_and_sidetone_key(0);
}
#endif

//---------------------------------------------------------------------

#if defined(FEATURE_SERIAL) && defined(FEATURE_COMMAND_LINE_INTERFACE)
void serial_status(PRIMARY_SERIAL_CLS *port_to_use)
{

  port_to_use->println();
#if defined(FEATURE_AMERICAN_MORSE)
  if (char_send_mode == AMERICAN_MORSE)
  {
    port_to_use->println(F("American Morse"));
  }
#endif
#if defined(FEATURE_HELL)
  if (char_send_mode == HELL)
  {
    port_to_use->println(F("Hellschreiber"));
  }
#endif
  switch (configuration.keyer_mode)
  {
  case IAMBIC_A:
    port_to_use->print(F("Iambic A"));
    break;
  case IAMBIC_B:
    port_to_use->print(F("Iambic B"));
#ifdef FEATURE_CMOS_SUPER_KEYER_IAMBIC_B_TIMING
    port_to_use->print(F(" / CMOS Super Keyer Timing: O"));
    if (configuration.cmos_super_keyer_iambic_b_timing_on)
    {
      port_to_use->print("n ");
      port_to_use->print(configuration.cmos_super_keyer_iambic_b_timing_percent);
      port_to_use->print("%");
    }
    else
    {
      port_to_use->print(F("ff"));
    }
#endif //FEATURE_CMOS_SUPER_KEYER_IAMBIC_B_TIMING
    break;
  case BUG:
    port_to_use->print(F("Bug"));
    break;
  case STRAIGHT:
    port_to_use->print(F("Straight Key"));
    break;
  case ULTIMATIC:
    port_to_use->print(F("Ultimatic "));
    switch (ultimatic_mode)
    {
    // case ULTIMATIC_NORMAL:
    //   port_to_use->print(F("Normal"));
    //   break;
    case ULTIMATIC_DIT_PRIORITY:
      port_to_use->print(F("Dit Priority"));
      break;
    case ULTIMATIC_DAH_PRIORITY:
      port_to_use->print(F("Dah Priority"));
      break;
    }
    break;
  case SINGLE_PADDLE:
    port_to_use->print(F("Single Paddle"));
    break;

    break;
  }
  port_to_use->println();
  port_to_use->print(F("Buffers: Dit O"));
  if (configuration.dit_buffer_off)
  {
    port_to_use->print(F("ff"));
  }
  else
  {
    port_to_use->print(F("n"));
  }
  port_to_use->print(F(" Dah O"));
  if (configuration.dah_buffer_off)
  {
    port_to_use->println(F("ff"));
  }
  else
  {
    port_to_use->println(F("n"));
  }
  if (speed_mode == SPEED_NORMAL)
  {
    port_to_use->print(F("WPM: "));
    port_to_use->println(configuration.wpm, DEC);
    port_to_use->print(F("Command Mode WPM: "));
    port_to_use->println(configuration.wpm_command_mode, DEC);
#ifdef FEATURE_FARNSWORTH
    port_to_use->print(F("Farnsworth WPM: "));
    if (configuration.wpm_farnsworth < configuration.wpm)
    {
      port_to_use->println(F("Disabled")); //(WD9DMP)
    }
    else
    {
      port_to_use->println(configuration.wpm_farnsworth, DEC);
    }
#endif //FEATURE_FARNSWORTH
  }
  else
  {
    port_to_use->print(F("QRSS Mode Activated - Dit Length: "));
    port_to_use->print(qrss_dit_length, DEC);
    port_to_use->println(F(" seconds"));
  }
  port_to_use->print(F("Sidetone: "));
  switch (configuration.sidetone_mode)
  {
  case SIDETONE_ON:
    port_to_use->print(F("On"));
    break; //(WD9DMP)
  case SIDETONE_OFF:
    port_to_use->print(F("Off"));
    break; //(WD9DMP)
  case SIDETONE_PADDLE_ONLY:
    port_to_use->print(F("Paddle Only"));
    break;
  }
  port_to_use->print(" ");
  port_to_use->print(configuration.hz_sidetone, DEC);
  port_to_use->println(" hz");

  // #if defined(FEATURE_SINEWAVE_SIDETONE)
  //   port_to_use->print(F("Sidetone Volume: "));
  //   port_to_use->print(map(configuration.sidetone_volume,sidetone_volume_low_limit,sidetone_volume_high_limit,0,100));
  //   port_to_use->println(F("%"));
  //   port_to_use->println(configuration.sidetone_volume);
  // #endif //FEATURE_SINEWAVE_SIDETONE

#ifdef FEATURE_SIDETONE_SWITCH
  port_to_use->print(F("Sidetone Switch: "));
  port_to_use->println(sidetone_switch_value() ? F("On") : F("Off")); //(WD9DMP)
#endif                                                                // FEATURE_SIDETONE_SWITCH
  port_to_use->print(F("Dah to dit: "));
  port_to_use->println((float(configuration.dah_to_dit_ratio) / 100));
  port_to_use->print(F("Weighting: "));
  port_to_use->println(configuration.weighting, DEC);
  port_to_use->print(F("Serial Number: "));
  port_to_use->println(serial_number, DEC);
#ifdef FEATURE_POTENTIOMETER
  port_to_use->print(F("Potentiometer WPM: "));
  port_to_use->print(pot_value_wpm(), DEC);
  port_to_use->print(F(" ("));
  if (configuration.pot_activated != 1)
  {
    port_to_use->print(F("Not "));
  }
  port_to_use->print(F("Activated) Range: "));
  port_to_use->print(pot_wpm_low_value);
  port_to_use->print(" - ");
  port_to_use->print(pot_wpm_high_value);
  port_to_use->println(F(" WPM"));
#endif
#ifdef FEATURE_AUTOSPACE
  port_to_use->print(F("Autospace O"));
  if (configuration.autospace_active)
  {
    port_to_use->println(F("n"));
  }
  else
  {
    port_to_use->println(F("ff"));
  }
#endif
  port_to_use->print(F("Wordspace: "));
  port_to_use->println(configuration.length_wordspace, DEC);
  port_to_use->print(F("TX: "));
  port_to_use->println(configuration.current_tx);

#ifdef FEATURE_QLF
  port_to_use->print(F("QLF: O"));
  if (qlf_active)
  {
    port_to_use->println(F("n"));
  }
  else
  {
    port_to_use->println(F("ff"));
  }
#endif //FEATURE_QLF

  port_to_use->print(F("Quiet Paddle Interrupt: "));
  if (configuration.paddle_interruption_quiet_time_element_lengths > 0)
  {
    port_to_use->println(configuration.paddle_interruption_quiet_time_element_lengths);
  }
  else
  {
    port_to_use->println(F("Off"));
  }

#if !defined(OPTION_EXCLUDE_MILL_MODE)
  port_to_use->print(F("Mill Mode: O"));
  if (configuration.cli_mode == CLI_NORMAL_MODE)
  {
    port_to_use->println(F("ff"));
  }
  else
  {
    port_to_use->println(F("n"));
  }
#endif // !defined(OPTION_EXCLUDE_MILL_MODE)

  port_to_use->print(F("PTT buffered character hold active: O"));
  if (configuration.ptt_buffer_hold_active)
  {
    port_to_use->println(F("n"));
  }
  else
  {
    port_to_use->println(F("ff"));
  }

  port_to_use->print(F("TX Inhibit: O"));
  if ((digitalRead(tx_inhibit_pin) == tx_inhibit_pin_active_state))
  {
    port_to_use->println(F("n"));
  }
  else
  {
    port_to_use->println(F("ff"));
  }
  port_to_use->print(F("TX Pause: O"));
  if ((digitalRead(tx_pause_pin) == tx_pause_pin_active_state))
  {
    port_to_use->println(F("n"));
  }
  else
  {
    port_to_use->println(F("ff"));
  }

#if defined(FEATURE_PADDLE_ECHO)
  port_to_use->print(F("Paddle Echo: O"));
  if (cli_paddle_echo)
  {
    port_to_use->println(F("n"));
  }
  else
  {
    port_to_use->println(F("ff"));
  }
#endif

#if defined(FEATURE_STRAIGHT_KEY_ECHO)
  port_to_use->print(F("Straight Key Echo: O"));
  if (cli_straight_key_echo)
  {
    port_to_use->println(F("n"));
  }
  else
  {
    port_to_use->println(F("ff"));
  }
#endif

  port_to_use->println(F(">"));
}
#endif

//---------------------------------------------------------------------

//---------------------------------------------------------------------

int convert_cw_number_to_ascii(long number_in)
{

  // number_in:  1 = dit, 2 = dah, 9 = a space

  switch (number_in)
  {
  case 12:
    return 65;
    break; // A
  case 2111:
    return 66;
    break;
  case 2121:
    return 67;
    break;
  case 211:
    return 68;
    break;
  case 1:
    return 69;
    break;
  case 1121:
    return 70;
    break;
  case 221:
    return 71;
    break;
  case 1111:
    return 72;
    break;
  case 11:
    return 73;
    break;
  case 1222:
    return 74;
    break;
  case 212:
    return 75;
    break;
  case 1211:
    return 76;
    break;
  case 22:
    return 77;
    break;
  case 21:
    return 78;
    break;
  case 222:
    return 79;
    break;
  case 1221:
    return 80;
    break;
  case 2212:
    return 81;
    break;
  case 121:
    return 82;
    break;
  case 111:
    return 83;
    break;
  case 2:
    return 84;
    break;
  case 112:
    return 85;
    break;
  case 1112:
    return 86;
    break;
  case 122:
    return 87;
    break;
  case 2112:
    return 88;
    break;
  case 2122:
    return 89;
    break;
  case 2211:
    return 90;
    break; // Z

  case 22222:
    return 48;
    break; // 0
  case 12222:
    return 49;
    break;
  case 11222:
    return 50;
    break;
  case 11122:
    return 51;
    break;
  case 11112:
    return 52;
    break;
  case 11111:
    return 53;
    break;
  case 21111:
    return 54;
    break;
  case 22111:
    return 55;
    break;
  case 22211:
    return 56;
    break;
  case 22221:
    return 57;
    break;
  case 112211:
    return '?';
    break; // ?
  case 21121:
    return 47;
    break; // /
#if !defined(OPTION_PROSIGN_SUPPORT)
  case 2111212:
    return '*';
    break; // BK
#endif
    //    case 221122: return 44; break;  // ,
    //    case 221122: return '!'; break;  // ! sp5iou 20180328
  case 221122:
    return ',';
    break;
  case 121212:
    return '.';
    break;
  case 122121:
    return '@';
    break;
  case 222222:
    return 92;
    break; // special hack; six dahs = \ (backslash)
  case 21112:
    return '=';
    break; // BT
  //case 2222222: return '+'; break;
  case 9:
    return 32;
    break; // special 9 = space

#ifndef OPTION_PS2_NON_ENGLISH_CHAR_LCD_DISPLAY_SUPPORT
  case 12121:
    return '+';
    break;
#else
  case 211112:
    return 45;
    break; // - // sp5iou
  case 212122:
    return 33;
    break; // ! //sp5iou
  case 1112112:
    return 36;
    break; // $ //sp5iou
#if !defined(OPTION_PROSIGN_SUPPORT)
  case 12111:
    return 38;
    break; // & // sp5iou
#endif
  case 122221:
    return 39;
    break; // ' // sp5iou
  case 121121:
    return 34;
    break; // " // sp5iou
  case 112212:
    return 95;
    break; // _ // sp5iou
  case 212121:
    return 59;
    break; // ; // sp5iou
  case 222111:
    return 58;
    break; // : // sp5iou
  case 212212:
    return 41;
    break; // KK (stored as ascii ) ) // sp5iou
#if !defined(OPTION_PROSIGN_SUPPORT)
  case 111212:
    return 62;
    break; // SK (stored as ascii > ) // sp5iou
#endif
  case 12121:
    return 60;
    break; // AR (store as ascii < ) // sp5iou
#endif //OPTION_PS2_NON_ENGLISH_CHAR_LCD_DISPLAY_SUPPORT

#if defined(OPTION_PROSIGN_SUPPORT)
#if !defined(OPTION_NON_ENGLISH_EXTENSIONS)
  case 1212:
    return PROSIGN_AA;
    break;
#endif
  case 12111:
    return PROSIGN_AS;
    break;
  case 2111212:
    return PROSIGN_BK;
    break;
  case 21211211:
    return PROSIGN_CL;
    break;
  case 21212:
    return PROSIGN_CT;
    break;
  case 21221:
    return PROSIGN_KN;
    break;
  case 211222:
    return PROSIGN_NJ;
    break;
  case 111212:
    return PROSIGN_SK;
    break;
  case 11121:
    return PROSIGN_SN;
    break;
  case 11111111:
    return PROSIGN_HH;
    break; // iz0rus
#else      //OPTION_PROSIGN_SUPPORT
  case 21221:
    return 40;
    break; // (KN store as ascii ( ) //sp5iou //aaaaaaa
#endif     //OPTION_PROSIGN_SUPPORT

#ifdef OPTION_NON_ENGLISH_EXTENSIONS
  // for English/Cyrillic/Western European font LCD controller (HD44780UA02):
  case 12212:
    return 197;
    break; // 'Å' - AA_capital (OZ, LA, SM)
  //case 12212: return 192; break;   // 'À' - A accent
  case 1212:
    return 198;
    break; // 'Æ' - AE_capital   (OZ, LA)
  //case 1212: return 196; break;    // 'Ä' - A_umlaut (D, SM, OH, ...)
  case 2222:
    return 138;
    break; // CH  - (Russian letter symbol)
  case 22122:
    return 209;
    break; // 'Ñ' - (EA)
  //case 2221: return 214; break;    // 'Ö' – O_umlaut  (D, SM, OH, ...)
  //case 2221: return 211; break;    // 'Ò' - O accent
  case 2221:
    return 216;
    break; // 'Ø' - OE_capital    (OZ, LA)
  case 1122:
    return 220;
    break; // 'Ü' - U_umlaut     (D, ...)
  case 111111:
    return 223;
    break; // beta - double S    (D?, ...)
  case 21211:
    return 199;
    break; // Ç
  case 11221:
    return 208;
    break; // Ð
  case 12112:
    return 200;
    break; // È
  case 11211:
    return 201;
    break; // É
  case 221121:
    return 142;
    break; // Ž
#endif     //OPTION_NON_ENGLISH_EXTENSIONS

  default:
#ifdef OPTION_UNKNOWN_CHARACTER_ERROR_TONE
    boop();
#endif //OPTION_UNKNOWN_CHARACTER_ERROR_TONE
    return unknown_cw_character;
    break;
  }
}

//---------------------------------------------------------------------
void check_button0()
{
#ifdef FEATURE_COMMAND_BUTTONS
  if (analogbuttonread(0))
  {
    button0_buffer = 1;
  }
#endif
}

//---------------------------------------------------------------------
#if defined(FEATURE_MEMORIES) || defined(FEATURE_COMMAND_LINE_INTERFACE)
void send_serial_number(byte cut_numbers, int increment_serial_number, byte buffered_sending)
{

  String serial_number_string;

  serial_number_string = String(serial_number, DEC);
  if (serial_number_string.length() < 3)
  {
    if (cut_numbers)
    {
      if (buffered_sending)
      {
        add_to_send_buffer('T');
      }
      else
      {
        if (keyer_machine_mode != KEYER_COMMAND_MODE)
        {
          display_serial_number_character('T');
        } //Display the SN as well as play it unless playing back after programming for verification(WD9DMP)
        send_char('T', KEYER_NORMAL);
      }
    }
    else
    {
      if (buffered_sending)
      {
        add_to_send_buffer('0');
      }
      else
      {
        if (keyer_machine_mode != KEYER_COMMAND_MODE)
        {
          display_serial_number_character('0');
        } //Display the SN as well as play it unless playing back after programming for verification(WD9DMP)
        send_char('0', KEYER_NORMAL);
      }
    }
  }
  if (serial_number_string.length() == 1)
  {
    if (cut_numbers)
    {
      if (buffered_sending)
      {
        add_to_send_buffer('T');
      }
      else
      {
        if (keyer_machine_mode != KEYER_COMMAND_MODE)
        {
          display_serial_number_character('T');
        } //Display the SN as well as play it unless playing back after programming for verification(WD9DMP)
        send_char('T', KEYER_NORMAL);
      }
    }
    else
    {
      if (buffered_sending)
      {
        add_to_send_buffer('0');
      }
      else
      {
        if (keyer_machine_mode != KEYER_COMMAND_MODE)
        {
          display_serial_number_character('0');
        } //Display the SN as well as play it unless playing back after programming for verification(WD9DMP)
        send_char('0', KEYER_NORMAL);
      }
    }
  }
  for (unsigned int a = 0; a < serial_number_string.length(); a++)
  {
    if ((serial_number_string[a] == '0') && (cut_numbers))
    {
      if (buffered_sending)
      {
        add_to_send_buffer('T');
      }
      else
      {
        if (keyer_machine_mode != KEYER_COMMAND_MODE)
        {
          display_serial_number_character('T');
        } //Display the SN as well as play it unless playing back after programming for verification(WD9DMP)
        send_char('T', KEYER_NORMAL);
      }
    }
    else
    {
      if ((serial_number_string[a] == '9') && (cut_numbers))
      {
        if (buffered_sending)
        {
          add_to_send_buffer('N');
        }
        else
        {
          if (keyer_machine_mode != KEYER_COMMAND_MODE)
          {
            display_serial_number_character('N');
          } //Display the SN as well as play it unless playing back after programming for verification(WD9DMP)
          send_char('N', KEYER_NORMAL);
        }
      }
      else
      {
        if (buffered_sending)
        {
          add_to_send_buffer(serial_number_string[a]);
        }
        else
        {
          if (keyer_machine_mode != KEYER_COMMAND_MODE)
          {
            display_serial_number_character(serial_number_string[a]);
          } //Display the SN as well as play it unless playing back after programming for verification(WD9DMP)
          send_char(serial_number_string[a], KEYER_NORMAL);
        }
      }
    }
  }

  serial_number = serial_number + increment_serial_number;
}

#endif
//New function below to send serial number character to CLI as well as LCD (WD9DMP)
//---------------------------------------------------------------------
#if defined(FEATURE_MEMORIES) || defined(FEATURE_COMMAND_LINE_INTERFACE)
void display_serial_number_character(char snumchar)
{
#if defined(FEATURE_SERIAL)
#ifdef FEATURE_COMMAND_LINE_INTERFACE
  primary_serial_port->write(snumchar);
#ifdef FEATURE_COMMAND_LINE_INTERFACE_ON_SECONDARY_PORT
  secondary_serial_port->write(snumchar);
#endif //FEATURE_COMMAND_LINE_INTERFACE_ON_SECONDARY_PORT
#endif //FEATURE_COMMAND_LINE_INTERFACE
#endif // (FEATURE_SERIAL)
#ifdef FEATURE_DISPLAY
  if (lcd_send_echo)
  {
    display_scroll_print_char(snumchar);
    service_display();
  }
#endif // FEATURE_DISPLAY
}

#endif
//---------------------------------------------------------------------

//---------------------------------------------------------------------

void initialize_pins()
{

#if defined(ARDUINO_MAPLE_MINI) || defined(ARDUINO_GENERIC_STM32F103C) || defined(ESP32) //sp5iou 20180329
  pinMode(paddle_left, INPUT_PULLUP);
  pinMode(paddle_right, INPUT_PULLUP);
#else
  pinMode(paddle_left, INPUT);
  digitalWrite(paddle_left, HIGH);
  pinMode(paddle_right, INPUT);
  digitalWrite(paddle_right, HIGH);
#endif //defined (ARDUINO_MAPLE_MINI)||defined(ARDUINO_GENERIC_STM32F103C) sp5iou 20180329

#if defined(FEATURE_CAPACITIVE_PADDLE_PINS)
  if (capactive_paddle_pin_inhibit_pin)
  {
    pinMode(capactive_paddle_pin_inhibit_pin, INPUT);
    digitalWrite(capactive_paddle_pin_inhibit_pin, LOW);
  }
#endif //FEATURE_CAPACITIVE_PADDLE_PINS

  if (tx_key_line_1)
  {
    pinMode(tx_key_line_1, OUTPUT);
    digitalWrite(tx_key_line_1, tx_key_line_inactive_state);
  }
  if (tx_key_line_2)
  {
    pinMode(tx_key_line_2, OUTPUT);
    digitalWrite(tx_key_line_2, tx_key_line_inactive_state);
  }
  if (tx_key_line_3)
  {
    pinMode(tx_key_line_3, OUTPUT);
    digitalWrite(tx_key_line_3, tx_key_line_inactive_state);
  }
  if (tx_key_line_4)
  {
    pinMode(tx_key_line_4, OUTPUT);
    digitalWrite(tx_key_line_4, tx_key_line_inactive_state);
  }
  if (tx_key_line_5)
  {
    pinMode(tx_key_line_5, OUTPUT);
    digitalWrite(tx_key_line_5, tx_key_line_inactive_state);
  }
  if (tx_key_line_6)
  {
    pinMode(tx_key_line_6, OUTPUT);
    digitalWrite(tx_key_line_6, tx_key_line_inactive_state);
  }

  if (ptt_tx_1)
  {
    pinMode(ptt_tx_1, OUTPUT);
    digitalWrite(ptt_tx_1, ptt_line_inactive_state);
  }
  if (ptt_tx_2)
  {
    pinMode(ptt_tx_2, OUTPUT);
    digitalWrite(ptt_tx_2, ptt_line_inactive_state);
  }
  if (ptt_tx_3)
  {
    pinMode(ptt_tx_3, OUTPUT);
    digitalWrite(ptt_tx_3, ptt_line_inactive_state);
  }
  if (ptt_tx_4)
  {
    pinMode(ptt_tx_4, OUTPUT);
    digitalWrite(ptt_tx_4, ptt_line_inactive_state);
  }
  if (ptt_tx_5)
  {
    pinMode(ptt_tx_5, OUTPUT);
    digitalWrite(ptt_tx_5, ptt_line_inactive_state);
  }
  if (ptt_tx_6)
  {
    pinMode(ptt_tx_6, OUTPUT);
    digitalWrite(ptt_tx_6, ptt_line_inactive_state);
  }
  if (sidetone_line)
  {
    pinMode(sidetone_line, OUTPUT);
    digitalWrite(sidetone_line, LOW);
  }
  if (tx_key_dit)
  {
    pinMode(tx_key_dit, OUTPUT);
    digitalWrite(tx_key_dit, tx_key_dit_and_dah_pins_inactive_state);
  }
  if (tx_key_dah)
  {
    pinMode(tx_key_dah, OUTPUT);
    digitalWrite(tx_key_dah, tx_key_dit_and_dah_pins_inactive_state);
  }

  if (ptt_input_pin)
  {
    pinMode(ptt_input_pin, INPUT_PULLUP);
  }

  if (tx_inhibit_pin)
  {
    pinMode(tx_inhibit_pin, INPUT_PULLUP);
  }

  if (tx_pause_pin)
  {
    pinMode(tx_pause_pin, INPUT_PULLUP);
  }

  if (potentiometer_enable_pin)
  {
    pinMode(potentiometer_enable_pin, INPUT_PULLUP);
  }

} //initialize_pins()

//---------------------------------------------------------------------

//---------------------------------------------------------------------

//---------------------------------------------------------------------

void initialize_keyer_state()
{

  key_state = 0;
  key_tx = 1;

  configuration.wpm = initial_speed_wpm;
  pot_wpm_low_value = initial_pot_wpm_low_value;
  configuration.paddle_interruption_quiet_time_element_lengths = default_paddle_interruption_quiet_time_element_lengths;
  configuration.hz_sidetone = initial_sidetone_freq;
  configuration.memory_repeat_time = default_memory_repeat_time;
  configuration.cmos_super_keyer_iambic_b_timing_percent = default_cmos_super_keyer_iambic_b_timing_percent;
  configuration.dah_to_dit_ratio = initial_dah_to_dit_ratio;
  configuration.length_wordspace = default_length_wordspace;
  configuration.weighting = default_weighting;
  configuration.wordsworth_wordspace = default_wordsworth_wordspace;
  configuration.wordsworth_repetition = default_wordsworth_repetition;
  configuration.wpm_farnsworth = initial_speed_wpm;
  configuration.cli_mode = CLI_NORMAL_MODE;
  configuration.wpm_command_mode = initial_command_mode_speed_wpm;
  configuration.ptt_buffer_hold_active = 0;
  configuration.sidetone_volume = sidetone_volume_low_limit + ((sidetone_volume_high_limit - sidetone_volume_low_limit) / 2);

  configuration.ptt_lead_time[0] = initial_ptt_lead_time_tx1;
  configuration.ptt_tail_time[0] = initial_ptt_tail_time_tx1;
  configuration.ptt_lead_time[1] = initial_ptt_lead_time_tx2;
  configuration.ptt_tail_time[1] = initial_ptt_tail_time_tx2;
#if !defined(OPTION_SAVE_MEMORY_NANOKEYER)
  configuration.ptt_lead_time[2] = initial_ptt_lead_time_tx3;
  configuration.ptt_tail_time[2] = initial_ptt_tail_time_tx3;
  configuration.ptt_lead_time[3] = initial_ptt_lead_time_tx4;
  configuration.ptt_tail_time[3] = initial_ptt_tail_time_tx4;
  configuration.ptt_lead_time[4] = initial_ptt_lead_time_tx5;
  configuration.ptt_tail_time[4] = initial_ptt_tail_time_tx5;
  configuration.ptt_lead_time[5] = initial_ptt_lead_time_tx6;
  configuration.ptt_tail_time[5] = initial_ptt_tail_time_tx6;

  for (int x = 0; x < 5; x++)
  {
    configuration.ptt_active_to_sequencer_active_time[x] = 0;
    configuration.ptt_inactive_to_sequencer_inactive_time[x] = 0;
  }
#endif //OPTION_SAVE_MEMORY_NANOKEYER

#ifndef FEATURE_SO2R_BASE
  switch_to_tx_silent(1);
#endif
}

//---------------------------------------------------------------------
void initialize_potentiometer()
{

#ifdef FEATURE_POTENTIOMETER
  pinMode(potentiometer, INPUT);
  pot_wpm_high_value = initial_pot_wpm_high_value;
  last_pot_wpm_read = pot_value_wpm();
  configuration.pot_activated = 1;
#endif
}

//---------------------------------------------------------------------

//---------------------------------------------------------------------

void initialize_default_modes()
{

  // setup default modes
  keyer_machine_mode = KEYER_NORMAL;
  configuration.paddle_mode = PADDLE_NORMAL;
  configuration.keyer_mode = IAMBIC_B;
  configuration.sidetone_mode = SIDETONE_ON;

#ifdef initial_sidetone_mode
  configuration.sidetone_mode = initial_sidetone_mode;
#endif

  char_send_mode = CW;

#if defined(FEATURE_CMOS_SUPER_KEYER_IAMBIC_B_TIMING) && defined(OPTION_CMOS_SUPER_KEYER_IAMBIC_B_TIMING_ON_BY_DEFAULT) // DL1HTB initialize CMOS Super Keyer if feature is enabled
  configuration.cmos_super_keyer_iambic_b_timing_on = 1;
#endif //FEATURE_CMOS_SUPER_KEYER_IAMBIC_B_TIMING // #end DL1HTB initialize CMOS Super Keyer if feature is enabled

  delay(250); // wait a little bit for the caps to charge up on the paddle lines
}

//---------------------------------------------------------------------

void check_eeprom_for_initialization()
{

#if !defined(ESP32)
  // do an eeprom reset to defaults if paddles are squeezed
  if (paddle_pin_read(paddle_left) == LOW && paddle_pin_read(paddle_right) == LOW)
  {
    while (paddle_pin_read(paddle_left) == LOW && paddle_pin_read(paddle_right) == LOW)
    {
    }
    initialize_eeprom();
  }
#endif

  // read settings from eeprom and initialize eeprom if it has never been written to
  if (read_settings_from_eeprom())
  {
#if defined(HARDWARE_GENERIC_STM32F103C)
    EEPROM.init();   //sp5iou 20180328 to reinitialize / initialize EEPROM
    EEPROM.format(); //sp5iou 20180328 to reinitialize / format EEPROM
#endif
    initialize_eeprom();
  }
}

//---------------------------------------------------------------------

void initialize_eeprom()
{

  write_settings_to_eeprom(1);
  // #if defined(FEATURE_SINEWAVE_SIDETONE)
  //   initialize_tonsin();
  // #endif
  beep_boop();
  beep_boop();
  beep_boop();
}

//---------------------------------------------------------------------

void check_for_beacon_mode()
{

#ifndef OPTION_SAVE_MEMORY_NANOKEYER
  // check for beacon mode (paddle_left == low) or straight key mode (paddle_right == low)
  if (paddle_pin_read(paddle_left) == LOW)
  {
#ifdef FEATURE_BEACON
    keyer_machine_mode = BEACON;
#endif
  }
  else
  {
    if (paddle_pin_read(paddle_right) == LOW)
    {
      configuration.keyer_mode = STRAIGHT;
    }
  }
#endif //OPTION_SAVE_MEMORY_NANOKEYER
}

//---------------------------------------------------------------------

//---------------------------------------------------------------------

void initialize_serial_ports()
{

// initialize serial port
#if defined(FEATURE_SERIAL)

#if defined(FEATURE_WINKEY_EMULATION) && defined(FEATURE_COMMAND_LINE_INTERFACE) //--------------------------------------------

#ifdef FEATURE_COMMAND_BUTTONS
  if (analogbuttonread(0))
  {
#ifdef OPTION_PRIMARY_SERIAL_PORT_DEFAULT_WINKEY_EMULATION
    primary_serial_port_mode = SERIAL_CLI;
    primary_serial_port_baud_rate = PRIMARY_SERIAL_PORT_BAUD;
#else
    primary_serial_port_mode = SERIAL_WINKEY_EMULATION;
    primary_serial_port_baud_rate = WINKEY_DEFAULT_BAUD;
#endif //ifndef OPTION_PRIMARY_SERIAL_PORT_DEFAULT_WINKEY_EMULATION
  }
  else
  {
#ifdef OPTION_PRIMARY_SERIAL_PORT_DEFAULT_WINKEY_EMULATION
    primary_serial_port_mode = SERIAL_WINKEY_EMULATION;
    primary_serial_port_baud_rate = WINKEY_DEFAULT_BAUD;
#else
    primary_serial_port_mode = SERIAL_CLI;
    primary_serial_port_baud_rate = PRIMARY_SERIAL_PORT_BAUD;
#endif //ifndef OPTION_PRIMARY_SERIAL_PORT_DEFAULT_WINKEY_EMULATION
  }
  while (analogbuttonread(0))
  {
  }
#else //FEATURE_COMMAND_BUTTONS
#ifdef OPTION_PRIMARY_SERIAL_PORT_DEFAULT_WINKEY_EMULATION
  primary_serial_port_mode = SERIAL_WINKEY_EMULATION;
  primary_serial_port_baud_rate = WINKEY_DEFAULT_BAUD;
#else
  primary_serial_port_mode = SERIAL_CLI;
  primary_serial_port_baud_rate = PRIMARY_SERIAL_PORT_BAUD;
#endif //ifndef OPTION_PRIMARY_SERIAL_PORT_DEFAULT_WINKEY_EMULATION
#endif //FEATURE_COMMAND_BUTTONS
#endif //defined(FEATURE_WINKEY_EMULATION) && defined(FEATURE_COMMAND_LINE_INTERFACE)---------------------------------

#if !defined(FEATURE_WINKEY_EMULATION) && defined(FEATURE_COMMAND_LINE_INTERFACE)
  primary_serial_port_mode = SERIAL_CLI;
  primary_serial_port_baud_rate = PRIMARY_SERIAL_PORT_BAUD;
#endif //!defined(FEATURE_WINKEY_EMULATION) && defined(FEATURE_COMMAND_LINE_INTERFACE)

#if defined(FEATURE_WINKEY_EMULATION) && !defined(FEATURE_COMMAND_LINE_INTERFACE)
  primary_serial_port_mode = SERIAL_WINKEY_EMULATION;
  primary_serial_port_baud_rate = WINKEY_DEFAULT_BAUD;
#endif //defined(FEATURE_WINKEY_EMULATION) && !defined(FEATURE_COMMAND_LINE_INTERFACE)

  primary_serial_port = PRIMARY_SERIAL_PORT;

  primary_serial_port->begin(primary_serial_port_baud_rate);

#ifdef DEBUG_STARTUP
  debug_serial_port->println(F("setup: serial port opened"));
#endif //DEBUG_STARTUP

#if !defined(OPTION_SUPPRESS_SERIAL_BOOT_MSG) && defined(FEATURE_COMMAND_LINE_INTERFACE)
  if (primary_serial_port_mode == SERIAL_CLI)
  {
    primary_serial_port->print(F("\n\rK3NG Keyer Version "));
    primary_serial_port->write(CODE_VERSION);
    primary_serial_port->println();
#if defined(FEATURE_SERIAL_HELP)
    primary_serial_port->println(F("\n\rEnter \\? for help\n"));
#endif
    while (primary_serial_port->available())
    {
      primary_serial_port->read();
    } //clear out buffer at boot
  }
#ifdef DEBUG_MEMORYCHECK
  memorycheck();
#endif //DEBUG_MEMORYCHECK
#endif //!defined(OPTION_SUPPRESS_SERIAL_BOOT_MSG) && defined(FEATURE_COMMAND_LINE_INTERFACE)

#ifdef DEBUG_AUX_SERIAL_PORT
  debug_port = DEBUG_AUX_SERIAL_PORT;
  debug_serial_port->begin(DEBUG_AUX_SERIAL_PORT_BAUD);
  debug_serial_port->print("debug port open ");
  debug_serial_port->println(CODE_VERSION);
#endif //DEBUG_AUX_SERIAL_PORT

#ifdef FEATURE_COMMAND_LINE_INTERFACE_ON_SECONDARY_PORT
  secondary_serial_port = SECONDARY_SERIAL_PORT;
  secondary_serial_port->begin(SECONDARY_SERIAL_PORT_BAUD);
#if !defined(OPTION_SUPPRESS_SERIAL_BOOT_MSG)
  secondary_serial_port->print(F("\n\rK3NG Keyer Version "));
  secondary_serial_port->write(CODE_VERSION);
  secondary_serial_port->println();
#if defined(FEATURE_SERIAL_HELP)
  secondary_serial_port->println(F("\n\rEnter \\? for help\n"));
#endif
  while (secondary_serial_port->available())
  {
    secondary_serial_port->read();
  } //clear out buffer at boot
#endif
#endif //FEATURE_COMMAND_LINE_INTERFACE_ON_SECONDARY_PORT

#ifdef FEATURE_COMMAND_LINE_INTERFACE_ON_SECONDARY_PORT
  debug_serial_port = secondary_serial_port;
#else
  debug_serial_port = primary_serial_port;
#endif

#endif //FEATURE_SERIAL
}

void initialize_display()
{

#ifdef FEATURE_DISPLAY
#if defined(FEATURE_LCD_SAINSMART_I2C)
  lcd.begin();
  lcd.home();
#else
  //lcd.begin(LCD_COLUMNS, LCD_ROWS);
  // initialize LCD
  Wire.begin(I2C_SDA, I2C_SCL);
  lcd.init();
  // turn on LCD backlight
  lcd.backlight();
#endif
#ifdef FEATURE_LCD_ADAFRUIT_I2C
  lcd.setBacklight(lcdcolor);
#endif //FEATURE_LCD_ADAFRUIT_I2C

#ifdef FEATURE_LCD_ADAFRUIT_BACKPACK
  lcd.setBacklight(HIGH);
#endif
#ifdef FEATURE_LCD_MATHERTEL_PCF8574
  lcd.setBacklight(HIGH);
#endif

#ifdef OPTION_DISPLAY_NON_ENGLISH_EXTENSIONS // OZ1JHM provided code, cleaned up by LA3ZA
  // Store bit maps, designed using editor at http://omerk.github.io/lcdchargen/

  byte U_umlaut[8] = {B01010, B00000, B10001, B10001, B10001, B10001, B01110, B00000};   // 'Ü'
  byte O_umlaut[8] = {B01010, B00000, B01110, B10001, B10001, B10001, B01110, B00000};   // 'Ö'
  byte A_umlaut[8] = {B01010, B00000, B01110, B10001, B11111, B10001, B10001, B00000};   // 'Ä'
  byte AE_capital[8] = {B01111, B10100, B10100, B11110, B10100, B10100, B10111, B00000}; // 'Æ'
  byte OE_capital[8] = {B00001, B01110, B10011, B10101, B11001, B01110, B10000, B00000}; // 'Ø'
  byte empty[8] = {B00000, B00000, B00000, B00000, B00000, B00000, B00000, B00000};      // empty
  byte AA_capital[8] = {B00100, B00000, B01110, B10001, B11111, B10001, B10001, B00000}; // 'Å'
  byte Ntilde[8] = {B01101, B10010, B00000, B11001, B10101, B10011, B10001, B00000};     // 'Ñ'

  //     upload 8 charaters to the lcd
  lcd.createChar(0, U_umlaut);   //     German
  lcd.createChar(1, O_umlaut);   //     German, Swedish
  lcd.createChar(2, A_umlaut);   //     German, Swedish
  lcd.createChar(3, AE_capital); //   Danish, Norwegian
  lcd.createChar(4, OE_capital); //   Danish, Norwegian
  lcd.createChar(5, empty);      //        For some reason this one needs to display nothing - otherwise it will display in pauses on serial interface
  lcd.createChar(6, AA_capital); //   Danish, Norwegian, Swedish
  lcd.createChar(7, Ntilde);     //       Spanish
  lcd.clear();                   // you have to ;o)
#endif                           //OPTION_DISPLAY_NON_ENGLISH_EXTENSIONS

  if (LCD_COLUMNS < 9)
  {
    lcd_center_print_timed("K3NGKeyr", 0, 4000);
  }
  else
  {
    lcd_center_print_timed("K3NG Keyer", 0, 4000);
#ifdef OPTION_PERSONALIZED_STARTUP_SCREEN
    if (LCD_ROWS == 2)
    {
#ifdef OPTION_DO_NOT_SAY_HI                                  // if we wish to display the custom field on the second line, we can't say 'hi'
      lcd_center_print_timed(custom_startup_field, 1, 4000); // display the custom field on the second line of the display, maximum field length is the number of columns
#endif                                                       // OPTION_DO_NOT_SAY_HI
    }
    else if (LCD_ROWS > 2)
      lcd_center_print_timed(custom_startup_field, 2, 4000); // display the custom field on the third line of the display, maximum field length is the number of columns
#endif                                                       // OPTION_PERSONALIZED_STARTUP_SCREEN
    if (LCD_ROWS > 3)
      lcd_center_print_timed("V: " + String(CODE_VERSION), 3, 4000); // display the code version on the fourth line of the display
  }
#endif //FEATURE_DISPLAY

  if (keyer_machine_mode != BEACON)
  {
#ifndef OPTION_DO_NOT_SAY_HI
    // say HI
    // store current setting (compliments of DL2SBA - http://dl2sba.com/ )
    byte oldKey = key_tx;
    byte oldSideTone = configuration.sidetone_mode;
    key_tx = 0;
    configuration.sidetone_mode = SIDETONE_ON;
#ifdef FEATURE_DISPLAY
    lcd_center_print_timed("h", 1, 4000);
#endif
    send_char('H', KEYER_NORMAL);
#ifdef FEATURE_DISPLAY
    lcd_center_print_timed("hi", 1, 4000);
#endif
    send_char('I', KEYER_NORMAL);
    configuration.sidetone_mode = oldSideTone;
    key_tx = oldKey;
#endif //OPTION_DO_NOT_SAY_HI
#ifdef OPTION_BLINK_HI_ON_PTT
    blink_ptt_dits_and_dahs(".... ..");
#endif
  }
}

int paddle_pin_read(int pin_to_read)
{

  return digitalRead(pin_to_read);
}

void service_millis_rollover()
{

  static unsigned long last_millis = 0;

  if (millis() < last_millis)
  {
    millis_rollover++;
  }
  last_millis = millis();
}

byte is_visible_character(byte char_in)
{

  if ((char_in > 31) || (char_in == 9) || (char_in == 10) || (char_in == 13))
  {
    return 1;
  }
  else
  {
    return 0;
  }
}
