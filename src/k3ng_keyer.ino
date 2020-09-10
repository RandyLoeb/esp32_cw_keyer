#define CODE_VERSION "2020.02.13.01"
#define M5CORE
#define eeprom_magic_number 35 // you can change this number to have the unit re-initialize EEPROM

#include <stdio.h>
#include <queue>
#include <string>
#include "keyer_hardware.h"
#include "keyer_features_and_options.h"
#include "keyer.h"
#include "keyer_dependencies.h"
#include "keyer_debug.h"
#include "keyer_pin_settings.h"
#include "keyer_settings.h"
#include "display.h"
#include "config.h"
#if !defined M5CORE
#include "potentiometer.h"
#endif
#include "cw_utils.h"

#define memory_area_start 113

#if defined(ESP32)
void add_to_send_buffer(byte incoming_serial_byte);

#if defined M5CORE
#include <M5Stack.h>
#endif

#if !defined M5CORE
#include "keyer_esp32now.h"
#endif

#include "keyer_esp32.h"
#endif

// Different hardware might need different tone control capabilities,
// so the approach is to use a global object that implements
// .tone and .noTone.
#if defined M5CORE
#include "tone/M5Tone.h"
M5Tone derivedFromToneBase;
#endif

// make sure your derived object is named derivedFromToneBase
// and implements toneBase's .tone and .noTone
toneBase &toneControl{derivedFromToneBase};

// Different ways to persist config
// E.g. if you wanted sd card inherit from persitentConfig
#if defined(ESP32)
#include "persistentConfig/spiffsPersistentConfig.h"
SpiffsPersistentConfig persistConfig;
#endif

// make sure your derived object is named persistConfig
// and implements .initialize and .save
persistentConfig &configControl{persistConfig};

#include "displays/keyerDisplay.h"

#include "virtualPins/virtualPins.h"
VirtualPins virtualPins;
#include "virtualPins/m5VirtualButtonPin.h"
M5VirtualButtonPin btnA{M5Btnslist::A};
M5VirtualButtonPin btnC{M5Btnslist::C};
#include "virtualPins/virtualArduinoStyleInputPin.h"
VirtualArduinoStyleInputPin p2(2, VitualArduinoPinPull::VAPP_HIGH);
VirtualArduinoStyleInputPin p5(5, VitualArduinoPinPull::VAPP_HIGH);

VirtualPin &vpA{btnA};
VirtualPin &vpC{btnC};
VirtualPin &vp2{p2};
VirtualPin &vp5{p5};

#include "virtualPins/virtualPinSet.h"
VirtualPinSet ditPaddles;
VirtualPinSet dahPaddles;

// move this later
void lcd_center_print_timed_wpm();

#include "webServer/wifiUtils.h"
WifiUtils wifiUtils{};

#include "webServer/keyerWebServer.h"
KeyerWebServer *keyerWebServer;

std::queue<String> injectedText;

#include "paddleMonitoring/paddleReader.h"
PaddleReader *paddleReader;

#include "timerStuff/timerStuff.h"

void setup()
{

#if defined M5CORE
  M5.begin();
  Serial.begin(115200);
  Serial.println("In setup()");
#endif

  //the web server needs a wifiutils object to handle wifi config
  //also needs place to inject text
  keyerWebServer = new KeyerWebServer(&wifiUtils, &injectedText, &configControl);

  ditPaddles.pins.push_back(&vpA);
  ditPaddles.pins.push_back(&vp2);
  dahPaddles.pins.push_back(&vpC);
  dahPaddles.pins.push_back(&vp5);

  virtualPins.pinsets.insert(std::make_pair(paddle_left, &ditPaddles));
  virtualPins.pinsets.insert(std::make_pair(paddle_right, &dahPaddles));

#ifdef ESPNOW_WIRELESS_KEYER
#ifndef M5CORE
  initialize_espnow_wireless(speed_set);
#endif
#endif
  //initialize_pins();

  initialize_keyer_state();
  configControl.initialize(primary_serial_port);

#if !defined M5CORE
  // potentiometer
  wpmPot.initialize(potentiometer_pin);
  configControl.configuration.pot_activated = 1;
#endif

  initialize_default_modes();
  //configControl.configuration.wpm = 15;
  //check_for_beacon_mode();
  displayControl.initialize([](char x) {
    send_char(x, KEYER_NORMAL);
  });
  wifiUtils.initialize();

  keyerWebServer->start();
  /* paddleReader = new PaddleReader([]() { return virtualPins.pinsets.at(paddle_left)->digRead(); },
                                  []() { return virtualPins.pinsets.at(paddle_right)->digRead(); },
                                  configControl.configuration.wpm); */

  // put your setup code here, to run once:
  initializeTimerStuff();

  // turn on the speaker
  M5.Speaker.begin();
}

// --------------------------------------------------------------------------------------------

void loop()
{
  detectInterrupts = true;
  /* check_paddles();

  service_dit_dah_buffers();

  service_send_buffer(PRINTCHAR);
  check_ptt_tail(); */

#if !defined M5CORE
  wpmPot.checkPotentiometer(wpmSetCallBack);
#endif

  check_for_dirty_configuration();

  /* check_paddles();
  service_send_buffer(PRINTCHAR); */
  displayControl.service_display();

  //service_paddle_echo();

  service_millis_rollover();
  wifiUtils.processNextDNSRequest();
  keyerWebServer->handleClient();
  //service_injected_text();

  processDitDahQueue();
}



void check_for_dirty_configuration()
{

  if (configControl.config_dirty)
  {
    Serial.println("dirty config!");
    save_persistent_configuration();
  }
}

//-------------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------------



//-------------------------------------------------------------------------------------------------------


//-------------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------------
void save_persistent_configuration()
{
  configControl.save();

  configControl.config_dirty = 0;
}



void speed_change(int change)
{
  if (((configControl.configuration.wpm + change) > wpm_limit_low) && ((configControl.configuration.wpm + change) < wpm_limit_high))
  {
    speed_set(configControl.configuration.wpm + change);
  }

  lcd_center_print_timed_wpm();
}

//-------------------------------------------------------------------------------------------------------

void speed_change_command_mode(int change)
{
  if (((configControl.configuration.wpm_command_mode + change) > wpm_limit_low) && ((configControl.configuration.wpm_command_mode + change) < wpm_limit_high))
  {
    configControl.configuration.wpm_command_mode = configControl.configuration.wpm_command_mode + change;
    configControl.config_dirty = 1;
  }

  displayControl.lcd_center_print_timed(String(configControl.configuration.wpm_command_mode) + " wpm", 0, default_display_msg_delay);
}

//-------------------------------------------------------------------------------------------------------



void adjust_dah_to_dit_ratio(int adjustment)
{

  if ((configControl.configuration.dah_to_dit_ratio + adjustment) > 150 && (configControl.configuration.dah_to_dit_ratio + adjustment) < 810)
  {
    configControl.configuration.dah_to_dit_ratio = configControl.configuration.dah_to_dit_ratio + adjustment;
#ifdef FEATURE_DISPLAY
#ifdef OPTION_MORE_DISPLAY_MSGS
    if (LCD_COLUMNS < 9)
    {
      displayControl.lcd_center_print_timed("DDR:" + String(configControl.configuration.dah_to_dit_ratio), 0, default_display_msg_delay);
    }
    else
    {
      displayControl.lcd_center_print_timed("Dah/Dit: " + String(configControl.configuration.dah_to_dit_ratio), 0, default_display_msg_delay);
    }
    service_display();
#endif
#endif
  }

  configControl.config_dirty = 1;
}

//-------------------------------------------------------------------------------------------------------

void sidetone_adj(int hz)
{

  if (((configControl.configuration.hz_sidetone + hz) > sidetone_hz_limit_low) && ((configControl.configuration.hz_sidetone + hz) < sidetone_hz_limit_high))
  {
    configControl.configuration.hz_sidetone = configControl.configuration.hz_sidetone + hz;
    configControl.config_dirty = 1;
#if defined(OPTION_MORE_DISPLAY_MSGS)
    if (LCD_COLUMNS < 9)
    {
      displayControl.lcd_center_print_timed(String(configControl.configuration.hz_sidetone) + " Hz", 0, default_display_msg_delay);
    }
    else
    {
      displayControl.lcd_center_print_timed("Sidetone " + String(configControl.configuration.hz_sidetone) + " Hz", 0, default_display_msg_delay);
    }
#endif
  }
}

//-------------------------------------------------------------------------------------------------------

//------------------------------------------------------------------



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
      //Serial.println("About to send dit...");
      //send_dit();
      break;
    case '-':
      //send_dah();
      break;
#if defined(FEATURE_AMERICAN_MORSE) // this is a bit of a hack, but who cares!  :-)
    case '~':

      being_sent = SENDING_DAH;
      tx_and_sidetone_key(1);
      if ((tx_key_dah) && (key_tx))
      {
        digitalWrite(tx_key_dah, tx_key_dit_and_dah_pins_active_state);
      }
      loop_element_lengths((float(4.0) * (float(configControl.configuration.weighting) / 50)), keying_compensation, configControl.configuration.wpm);
      if ((tx_key_dah) && (key_tx))
      {
        digitalWrite(tx_key_dah, tx_key_dit_and_dah_pins_inactive_state);
      }
      tx_and_sidetone_key(0);
      loop_element_lengths((4.0 - (3.0 * (float(configControl.configuration.weighting) / 50))), (-1.0 * keying_compensation), configControl.configuration.wpm);
      break;

    case '=':
      being_sent = SENDING_DAH;
      tx_and_sidetone_key(1);
      if ((tx_key_dah) && (key_tx))
      {
        digitalWrite(tx_key_dah, tx_key_dit_and_dah_pins_active_state);
      }
      loop_element_lengths((float(5.0) * (float(configControl.configuration.weighting) / 50)), keying_compensation, configControl.configuration.wpm);
      if ((tx_key_dah) && (key_tx))
      {
        digitalWrite(tx_key_dah, tx_key_dit_and_dah_pins_inactive_state);
      }
      tx_and_sidetone_key(0);
      loop_element_lengths((4.0 - (3.0 * (float(configControl.configuration.weighting) / 50))), (-1.0 * keying_compensation), configControl.configuration.wpm);
      break;

    case '&':
      loop_element_lengths((4.0 - (3.0 * (float(configControl.configuration.weighting) / 50))), (-1.0 * keying_compensation), configControl.configuration.wpm);
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

  if ((cw_char == 10) || (cw_char == 13))
  {
    return;
  } // don't attempt to send carriage return or line feed

  sending_mode = AUTOMATIC_SENDING;

  if (char_send_mode == CW)
  {
    bool cwHandled = false;

    //special cases
    switch (cw_char)
    {
    case '\n':
      cwHandled = true;
      break;
    case '\r':
      cwHandled = true;
      break;
    case ' ':
      //loop_element_lengths((configControl.configuration.length_wordspace - length_letterspace - 2), 0, configControl.configuration.wpm);
      cwHandled = true;
      break;
    case '|':
#if !defined(OPTION_WINKEY_DO_NOT_SEND_7C_BYTE_HALF_SPACE)
      //loop_element_lengths(0.5, 0, configControl.configuration.wpm);
#endif
      cwHandled = true;
      break;
    default:
      cwHandled = false;
      break;
    }

    //most cases
    if (!cwHandled)
    {
      //get the dits and dahs
      String ditsanddahs = "" + convertCharToDitsAndDahs(cw_char);
      if (ditsanddahs.length() > 0)
      {
        send_the_dits_and_dahs(ditsanddahs.c_str());
      }
    }

    if (omit_letterspace != OMIT_LETTERSPACE)
    {

      //loop_element_lengths((length_letterspace - 1), 0, configControl.configuration.wpm); //this is minus one because send_dit and send_dah have a trailing element space
    }

#ifdef FEATURE_FARNSWORTH
    // Farnsworth Timing : http://www.arrl.org/files/file/Technology/x9004008.pdf
    if (configControl.configuration.wpm_farnsworth > configControl.configuration.wpm)
    {
      float additional_intercharacter_time_ms = ((((1.0 * farnsworth_timing_calibration) * ((60.0 * float(configControl.configuration.wpm_farnsworth)) - (37.2 * float(configControl.configuration.wpm))) / (float(configControl.configuration.wpm) * float(configControl.configuration.wpm_farnsworth))) / 19.0) * 1000.0) - (1200.0 / float(configControl.configuration.wpm_farnsworth));
      //loop_element_lengths(1, additional_intercharacter_time_ms, 0);
    }
#endif
  }
}

//-------------------------------------------------------------------------------------------------------


void service_injected_text()
{
  while (!injectedText.empty())
  {
    char x;
    String s = injectedText.front();
    s.toUpperCase();
    for (int i = 0; i < s.length(); i++)
    {
      x = s.charAt(i);
      add_to_send_buffer(x);
    }
    injectedText.pop();
  }
}

void service_paddle_echo()
{
#ifdef FEATURE_PADDLE_ECHO
  static byte paddle_echo_space_sent = 1;

  if ((paddle_echo_buffer) && (millis() > paddle_echo_buffer_decode_time))
  {

#ifdef FEATURE_DISPLAY
    if (displayControl.lcd_paddle_echo)
    {

      displayControl.displayUpdate(convert_cw_number_to_string(paddle_echo_buffer));
    }
#endif //FEATURE_DISPLAY

    paddle_echo_buffer = 0;
    paddle_echo_buffer_decode_time = millis() + (float(600 / configControl.configuration.wpm) * length_letterspace);
    paddle_echo_space_sent = 0;
  }

  // is it time to echo a space?
  if ((paddle_echo_buffer == 0) && (millis() > (paddle_echo_buffer_decode_time + (float(1200 / configControl.configuration.wpm) * (configControl.configuration.length_wordspace - length_letterspace)))) && (!paddle_echo_space_sent))
  {

#ifdef FEATURE_DISPLAY
    if (displayControl.lcd_paddle_echo)
    {
      displayControl.displayUpdate(' ');
    }
#endif //FEATURE_DISPLAY

    paddle_echo_space_sent = 1;
  }
#endif //FEATURE_PADDLE_ECHO
}



//---------------------------------------------------------------------

//---------------------------------------------------------------------

//---------------------------------------------------------------------

void initialize_keyer_state()
{

  key_state = 0;
  key_tx = 1;

#ifndef FEATURE_SO2R_BASE
  //switch_to_tx_silent(1);
#endif
}

//---------------------------------------------------------------------

//---------------------------------------------------------------------

//---------------------------------------------------------------------

void initialize_default_modes()
{

  // setup default modes
  keyer_machine_mode = KEYER_NORMAL;

  char_send_mode = CW;

#if defined(FEATURE_CMOS_SUPER_KEYER_IAMBIC_B_TIMING) && defined(OPTION_CMOS_SUPER_KEYER_IAMBIC_B_TIMING_ON_BY_DEFAULT) // DL1HTB initialize CMOS Super Keyer if feature is enabled
  configControl.configuration.cmos_super_keyer_iambic_b_timing_on = 1;
#endif //FEATURE_CMOS_SUPER_KEYER_IAMBIC_B_TIMING // #end DL1HTB initialize CMOS Super Keyer if feature is enabled

  delay(250); // wait a little bit for the caps to charge up on the paddle lines
}

//---------------------------------------------------------------------

//---------------------------------------------------------------------

//---------------------------------------------------------------------



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

void lcd_center_print_timed_wpm()
{

#if defined(OPTION_ADVANCED_SPEED_DISPLAY)
  displayControl.lcd_center_print_timed(String(configuration.wpm) + " wpm - " + (configuration.wpm * 5) + " cpm", 0, default_display_msg_delay);
  displayControl.lcd_center_print_timed(String(1200 / configuration.wpm) + ":" + (((1200 / configuration.wpm) * configuration.dah_to_dit_ratio) / 100) + "ms 1:" + (float(configuration.dah_to_dit_ratio) / 100.00), 1, default_display_msg_delay);
#else
  displayControl.lcd_center_print_timed(String(configControl.configuration.wpm) + " wpm", 0, default_display_msg_delay);
#endif
}

void speed_set(int wpm_set)
{

  if ((wpm_set > 0) && (wpm_set < 1000))
  {
    configControl.configuration.wpm = wpm_set;
    configControl.config_dirty = 1;

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

    lcd_center_print_timed_wpm();
  }
}

void command_speed_set(int wpm_set)
{
  if ((wpm_set > 0) && (wpm_set < 1000))
  {
    configControl.configuration.wpm_command_mode = wpm_set;
    configControl.config_dirty = 1;

    displayControl.lcd_center_print_timed("Cmd Spd " + String(configControl.configuration.wpm_command_mode) + " wpm", 0, default_display_msg_delay);

  } // end if
} // end command_speed_set

void wpmSetCallBack(byte pot_value_wpm_read)
{
  if (keyer_machine_mode == KEYER_COMMAND_MODE)
    command_speed_set(pot_value_wpm_read);
  else
    speed_set(pot_value_wpm_read);
}