#include "keyer_esp32.h"
#include <stdio.h>
#include <queue>
#include <string>
#include "keyer_features_and_options.h"
#include "keyer.h"
#include "keyer_pin_settings.h"
#include "keyer_settings.h"
#include "config.h"

#if defined M5CORE
#include <M5Stack.h>
//https://github.com/m5stack/M5Stack/issues/97
#undef min
#endif

#include "keyer_esp32now.h"

#include "tone/keyerTone.h"

// Different ways to persist config
// E.g. if you wanted sd card inherit from persitentConfig

#include "persistentConfig/spiffsPersistentConfig.h"
SpiffsPersistentConfig persistConfig;

// make sure your derived object is named persistConfig
// and implements .initialize and .save
persistentConfig &configControl{persistConfig};

#include "displays/keyerDisplay.h"

#include "webServer/wifiUtils.h"
WifiUtils wifiUtils{};

#include "webServer/webServer.h"

#include "timerStuff/timerStuff.h"
#include "virtualPins/keyerPins.h"

#include "cwControl/cw_utils.h"
CwControl *cwControl;
void setup()
{

#if defined M5CORE
  M5.begin();

#endif
  Serial.begin(115200);
  //Serial.begin(1200);

  cwControl = new CwControl(&ditsNdahQueue);
#if !defined ESPNOW_ONLY && defined KEYER_WEBSERVER
  //the web server needs a wifiutils object to handle wifi config
  //also needs place to inject text
  keyerWebServer = new KeyerWebServer(&wifiUtils, &cwControl->injectedText, &configControl, &displayCache);

#endif

  initialize_virtualPins();

  initialize_keyer_state();
  configControl.initialize();

  initialize_default_modes();

  // initialize tone stuff
  initializeTone();

#if !defined ESPNOW_ONLY

  wifiUtils.initialize();
  configControl.IPAddress = String(wifiUtils.getIp());
  configControl.MacAddress = String(wifiUtils.getMac());
#endif
#if !defined ESPNOW_ONLY && defined KEYER_WEBSERVER
  keyerWebServer->start();
#endif

#if defined ESPNOW_ONLY
  //wifiUtils.disconnectWiFi();

  wifiUtils.justStation();
#endif

#if defined ESPNOW

  initialize_espnow_wireless();
  _ditdahCallBack = &ditdahCallBack;
#endif

  initializeTimerStuff(&configControl, cwControl);
  detectInterrupts = true;

  displayControl.initialize([](char x) {
    cwControl->send_char(x, KEYER_NORMAL, false);
  });
}

// --------------------------------------------------------------------------------------------
int incomingByte = 0; // for incoming serial data
int firstWinkey = 0;
byte incoming_serial_byte;
void service_winkey(byte action);
int lastReadSerial = 0;
String winkeyStringBuffer = "";
int winkeyStringFlushLength = 0;
int waitPerChar = 1000;
int winkeyFlushTime = 0;

int isSerialSafe()
{
  return (millis() - winkeyFlushTime) > (winkeyStringFlushLength * waitPerChar);
}

void loop()
{

  if (configControl.config_dirty)
  {

    configControl.save();

    configControl.config_dirty = 0;
  }

  displayControl.service_display();

  service_millis_rollover();
  //wifiUtils.processNextDNSRequest();

  cwControl->service_injected_text();

  processDitDahQueue();

#if defined TEST_BEACON && !defined REMOTE_KEYER
  if (ditsNdahQueue.empty())
  {
    delay(5000);
    String testText = "THE QUICK BROWN FOX JUMPED OVER THE LAZY DOGS. 1234567890";
    cwControl.injectedText.push(testText);
  }
#endif

#ifdef ESP32WINKEYER
  if (isSerialSafe() && Serial.available() > 0)
  {
    // read the incoming byte:
    incoming_serial_byte = Serial.read();
    lastReadSerial = millis();
    //displayCache.add(String(incoming_serial_byte));
    //displayCache.add("\n");
    service_winkey(SERVICE_SERIAL_BYTE);
    /*
    if (incomingByte == 0x02)
    {
      //if (!firstWinkey)
      //{
        Serial.write(WINKEY_1_REPORT_VERSION_NUMBER);
        firstWinkey = 1;
      //}
    }
    */
  }

  if (winkeyStringBuffer.length() > 0 && millis() - lastReadSerial > timingControl.Farnsworth.wordSpace_ms)
  {
    cwControl->injectedText.push(winkeyStringBuffer);
    winkeyStringFlushLength = winkeyStringBuffer.length();
    winkeyStringBuffer = "";
    winkeyFlushTime = millis();
  }

  if (isSerialSafe())
  {
    service_winkey(WINKEY_HOUSEKEEPING);
  }
#endif
}

//-------------------------------------------------------------------------------------------------------

void initialize_keyer_state()
{

  key_state = 0;
  key_tx = 1;
}

void initialize_default_modes()
{

  // setup default modes
  keyer_machine_mode = KEYER_NORMAL;

  char_send_mode = CW;

  delay(250); // wait a little bit for the caps to charge up on the paddle lines
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

#define FEATURE_WINKEY_EMULATION 1
#ifdef FEATURE_WINKEY_EMULATION

byte winkey_serial_echo = 1;
byte winkey_host_open = 0;
unsigned int winkey_last_unbuffered_speed_wpm = 0;
byte winkey_speed_state = WINKEY_UNBUFFERED_SPEED;
byte winkey_buffer_counter = 0;
byte winkey_buffer_pointer = 0;
byte winkey_dit_invoke = 0;
byte winkey_dah_invoke = 0;
long winkey_paddle_echo_buffer = 0;
byte winkey_paddle_echo_activated = 0;
unsigned long winkey_paddle_echo_buffer_decode_time = 0;
byte winkey_sending = 0;
byte winkey_interrupted = 0;
byte winkey_xoff = 0;
byte winkey_session_ptt_tail = 0;
byte winkey_pinconfig_ptt_bit = 1;
#ifdef OPTION_WINKEY_SEND_BREAKIN_STATUS_BYTE
byte winkey_breakin_status_byte_inhibit = 0;
#endif //OPTION_WINKEY_SEND_BREAKIN_STATUS_BYTE
//define DEBUG_WINKEY 1
void winkey_port_write(byte byte_to_send, byte override_filter)
{

#ifdef DEBUG_WINKEY_PORT_WRITE
  if ((byte_to_send > 4) && (byte_to_send < 31))
  {
    boop();
    delay(500);
    boop();
    delay(500);
    boop();
    //return;
  }
#endif

  if (((byte_to_send > 4) && (byte_to_send < 31)) && (!override_filter))
  {
#ifdef DEBUG_WINKEY
    displayCache.add("Winkey Port TX: FILTERED: ");
    if ((byte_to_send > 31) && (byte_to_send < 127))
    {
      displayCache.add(String(byte_to_send));
    }
    else
    {
      displayCache.add(".");
    }
    displayCache.add(" [");
    displayCache.add(String(byte_to_send));
    displayCache.add("] [0x");
    displayCache.add(String(byte_to_send)); //, HEX);
    displayCache.add("]");
#endif
    return;
  }

  //primary_serial_port->write(byte_to_send);
  Serial.write(byte_to_send);
#ifdef DEBUG_WINKEY
  displayCache.add("Winkey Port TX: ");
  if ((byte_to_send > 31) && (byte_to_send < 127))
  {
    displayCache.add(String(byte_to_send));
  }
  else
  {
    displayCache.add(".");
  }
  displayCache.add(" [");
  displayCache.add(String(byte_to_send));
  displayCache.add("] [0x");
  displayCache.add(String(byte_to_send)); // HEX);
  displayCache.add("]");
#endif
}

byte wk2_mode = 1;

void service_winkey(byte action)
{

  static byte winkey_status = WINKEY_NO_COMMAND_IN_PROGRESS;
  static int winkey_parmcount = 0;
  static unsigned long winkey_last_activity;
  byte status_byte_to_send;
  static byte winkey_paddle_echo_space_sent = 1;

#ifdef OPTION_WINKEY_DISCARD_BYTES_AT_STARTUP
  static byte winkey_discard_bytes_init_done = 0;
  if (!winkey_discard_bytes_init_done)
  {
    if (primary_serial_port->available())
    {
      for (int z = winkey_discard_bytes_startup; z > 0; z--)
      {
        while (primary_serial_port->available() == 0)
        {
        }
        primary_serial_port->read();
      }
      winkey_discard_bytes_init_done = 1;
    }
  }
#endif //OPTION_WINKEY_DISCARD_BYTES_AT_STARTUP

#ifdef DEBUG_WINKEY_SEND_ERRANT_BYTE
  byte i_sent_it = 0;

  if ((millis() > 30000) && (!i_sent_it))
  {
    winkey_port_write(30, 1);
    i_sent_it = 1;
  }

#endif

#ifdef OPTION_WINKEY_IGNORE_FIRST_STATUS_REQUEST
  static byte ignored_first_status_request = 0;
#endif //OPTION_WINKEY_IGNORE_FIRST_STATUS_REQUEST

  if (action == WINKEY_HOUSEKEEPING)
  {
    if (winkey_last_unbuffered_speed_wpm == 0)
    {
      //REL winkey_last_unbuffered_speed_wpm = configuration.wpm;
    }

    // Winkey interface emulation housekeeping items
    // check to see if we were sending stuff and the buffer is clear
    if (winkey_interrupted)
    { // if Winkey sending was interrupted by the paddle, look at PTT line rather than timing out to send 0xc0
      if (ptt_line_activated == 0)
      {
#ifdef DEBUG_WINKEY
        displayCache.add(" service_winkey:sending unsolicited status byte due to paddle interrupt");
#endif //DEBUG_WINKEY
        winkey_sending = 0;
        //REL clear_send_buffer();

#ifdef FEATURE_MEMORIES
        //clear_memory_button_buffer();
        play_memory_prempt = 1;
        repeat_memory = 255;
#endif

        winkey_interrupted = 0;
        //winkey_port_write(0xc2|winkey_sending|winkey_xoff);
        winkey_port_write(0xc6, 0); //<- this alone makes N1MM logger get borked (0xC2 = paddle interrupt)
        winkey_port_write(0xc0, 0); // so let's send a 0xC0 to keep N1MM logger happy
        winkey_buffer_counter = 0;
        winkey_buffer_pointer = 0;
      }
    }
    else
    { //if (winkey_interrupted)
      //if ((winkey_host_open) && (winkey_sending) && (send_buffer_bytes < 1) && ((millis() - winkey_last_activity) > winkey_c0_wait_time)) {
      if ((Serial.available() == 0) && (winkey_host_open) && (winkey_sending) && (send_buffer_bytes < 1) && ((millis() - winkey_last_activity) > winkey_c0_wait_time))
      {
#ifdef OPTION_WINKEY_SEND_WORDSPACE_AT_END_OF_BUFFER
        send_char(' ', KEYER_NORMAL);
#endif
//add_to_send_buffer(' ');    // this causes a 0x20 to get echoed back to host - doesn't seem to effect N1MM program
#ifdef DEBUG_WINKEY
        displayCache.add(" service_winkey:sending unsolicited status byte");
#endif //DEBUG_WINKEY
        winkey_sending = 0;
        winkey_port_write(0xc0 | winkey_sending | winkey_xoff, 0); // tell the host we've sent everything
        winkey_buffer_counter = 0;
        winkey_buffer_pointer = 0;
      }
    } // if (winkey_interrupted)

    // failsafe check - if we've been in some command status for awhile waiting for something, clear things out
    if ((winkey_status != WINKEY_NO_COMMAND_IN_PROGRESS) && ((millis() - winkey_last_activity) > winkey_command_timeout_ms))
    {
#ifdef DEBUG_WINKEY
      displayCache.add(" service_winkey:cmd tout!->WINKEY_NO_COMMAND_IN_PROGRESS cmd was:");
      displayCache.add(String(winkey_status));
#endif //DEBUG_WINKEY
      winkey_status = WINKEY_NO_COMMAND_IN_PROGRESS;
      winkey_buffer_counter = 0;
      winkey_buffer_pointer = 0;
      winkey_port_write(0xc0 | winkey_sending | winkey_xoff, 0); //send a status byte back for giggles
    }

    /* REL */
    if ((winkey_host_open) && (winkey_paddle_echo_buffer) && (winkey_paddle_echo_activated) && (millis() > winkey_paddle_echo_buffer_decode_time))
    {
#ifdef DEBUG_WINKEY
      displayCache.add(" service_winkey:sending paddle echo char");
#endif //DEBUG_WINKEY
      //REL winkey_port_write(byte(convert_cw_number_to_ascii(winkey_paddle_echo_buffer)),0);
      winkey_paddle_echo_buffer = 0;
      //REL winkey_paddle_echo_buffer_decode_time = millis() + (float(600/configuration.wpm)*length_letterspace);
      winkey_paddle_echo_space_sent = 0;
    }

    if ((winkey_host_open) && (winkey_paddle_echo_buffer == 0) &&
        (winkey_paddle_echo_activated) &&
        (millis() > (winkey_paddle_echo_buffer_decode_time + (float(1200 / 25) * (1)))) && (!winkey_paddle_echo_space_sent))
    {
#ifdef DEBUG_WINKEY
      displayCache.add(" service_winkey:sending paddle echo space");
#endif //DEBUG_WINKEY
      winkey_port_write(' ', 0);
      winkey_paddle_echo_space_sent = 1;
    }

    /* REL  */
  } // if (action == WINKEY_HOUSEKEEPING)

  if (action == SERVICE_SERIAL_BYTE)
  {
#ifdef DEBUG_WINKEY
    displayCache.add("Winkey Port RX: ");
    if ((incoming_serial_byte > 31) && (incoming_serial_byte < 127))
    {
      displayCache.add(String(incoming_serial_byte));
    }
    else
    {
      displayCache.add(".");
    }
    displayCache.add(" [");
    displayCache.add(String(incoming_serial_byte));
    displayCache.add("]");
    displayCache.add(" [0x");
    if (incoming_serial_byte < 16)
    {
      displayCache.add("0");
    }
    displayCache.add(String(incoming_serial_byte)); //, HEX);
    displayCache.add("]");
#endif //DEBUG_WINKEY

    winkey_last_activity = millis();

    if (winkey_status == WINKEY_NO_COMMAND_IN_PROGRESS)
    {
#if defined(FEATURE_SO2R_BASE)
      if (incoming_serial_byte >= 128)
      {
        so2r_command();
      }
#endif //FEATURE_SO2R_BASE

#if defined(OPTION_WINKEY_EXTENDED_COMMANDS_WITH_DOLLAR_SIGNS)

#if !defined(OPTION_WINKEY_IGNORE_LOWERCASE)
      if ((incoming_serial_byte > 31) && (incoming_serial_byte != 36))
      { // ascii 36 = '$'
#else
      if ((((incoming_serial_byte > 31) && (incoming_serial_byte < 97)) || (incoming_serial_byte == 124)) && (incoming_serial_byte != 36))
      { // 124 = ascii | = half dit
#endif

#else

#if !defined(OPTION_WINKEY_IGNORE_LOWERCASE)
      if (incoming_serial_byte > 31)
      {
#else
      if (((incoming_serial_byte > 31) && (incoming_serial_byte < 97)) || (incoming_serial_byte == 124))
      { // 124 = ascii | = half dit
#endif

#endif

#if !defined(OPTION_WINKEY_IGNORE_LOWERCASE)
        if ((incoming_serial_byte > 96) && (incoming_serial_byte < 123))
        {
          incoming_serial_byte = incoming_serial_byte - 32;
        }
#endif //!defined(OPTION_WINKEY_IGNORE_LOWERCASE)

        byte serial_buffer_position_to_overwrite;

        if (winkey_buffer_pointer > 0)
        {
          serial_buffer_position_to_overwrite = send_buffer_bytes - (winkey_buffer_counter - winkey_buffer_pointer) - 1;
          if ((send_buffer_bytes) && (serial_buffer_position_to_overwrite < send_buffer_bytes))
          {

#ifdef DEBUG_WINKEY
            displayCache.add("service_winkey:serial_buffer_position_to_overwrite:");
            displayCache.add(String(serial_buffer_position_to_overwrite));
            displayCache.add(":");
            displayCache.add(String(incoming_serial_byte));
            //displayCache.add();
#endif //DEBUG_WINKEY
            send_buffer_array[serial_buffer_position_to_overwrite] = incoming_serial_byte;
          }
          winkey_buffer_pointer++;
        }
        else
        {

          //REL add_to_send_buffer(incoming_serial_byte);
          //displayCache.add("!!");

          //displayCache.add(String(incoming_serial_byte));
          //displayCache.add(String(char(incoming_serial_byte)));
          winkeyStringBuffer += String(char(incoming_serial_byte));

          // displayCache.add("\n");

#ifdef DEBUG_WINKEY
          displayCache.add("service_winkey:add_to_send_buffer:");
          displayCache.add(String(incoming_serial_byte));
          displayCache.add(" send_buffer_bytes:");
          displayCache.add(String(send_buffer_bytes));
#endif //DEBUG_WINKEY

#if defined(OPTION_WINKEY_INTERRUPTS_MEMORY_REPEAT) && defined(FEATURE_MEMORIES)
          play_memory_prempt = 1;
          repeat_memory = 255;
#endif
          winkey_buffer_counter++;
        }

        if (!winkey_sending)
        {
#ifdef DEBUG_WINKEY
          displayCache.add("service_winkey:status byte:starting to send");
#endif //DEBUG_WINKEY
          winkey_sending = 0x04;
#if !defined(OPTION_WINKEY_UCXLOG_SUPRESS_C4_STATUS_BYTE)
          winkey_port_write(0xc4 | winkey_sending | winkey_xoff, 0); // tell the client we're starting to send
#endif                                                               //OPTION_WINKEY_UCXLOG_SUPRESS_C4_STATUS_BYTE
#ifdef FEATURE_MEMORIES
          repeat_memory = 255;
#endif
        }
      }
      else
      {

#ifdef OPTION_WINKEY_STRICT_HOST_OPEN
        if ((winkey_host_open) || (incoming_serial_byte == 0))
        {
#endif

          switch (incoming_serial_byte)
          {
          case 0x00:
            winkey_status = WINKEY_ADMIN_COMMAND;
#ifdef DEBUG_WINKEY
            displayCache.add("service_winkey:ADMIN_CMD");
#endif //DEBUG_WINKEY
            break;
          case 0x01:
            winkey_status = WINKEY_SIDETONE_FREQ_COMMAND;
#ifdef DEBUG_WINKEY
            displayCache.add("service_winkey:WINKEY_SIDETONE_FREQ_COMMAND");
#endif //DEBUG_WINKEY
            break;
          case 0x02: // speed command - unbuffered
            winkey_status = WINKEY_UNBUFFERED_SPEED_COMMAND;
#ifdef DEBUG_WINKEY
            displayCache.add("service_winkey:WINKEY_UNBUFFERED_SPEED_COMMAND");
#endif //DEBUG_WINKEY
            break;
          case 0x03: // weighting
            winkey_status = WINKEY_WEIGHTING_COMMAND;
#ifdef DEBUG_WINKEY
            displayCache.add("service_winkey:WINKEY_WEIGHTING_COMMAND");
#endif //DEBUG_WINKEY
            break;
          case 0x04: // PTT lead and tail time
            winkey_status = WINKEY_PTT_TIMES_PARM1_COMMAND;
#ifdef DEBUG_WINKEY
            displayCache.add("service_winkey:WINKEY_PTT_TIMES_PARM1_COMMAND");
#endif //DEBUG_WINKEY
            break;
          case 0x05: // speed pot set
#ifdef DEBUG_WINKEY
            displayCache.add("service_winkey:WINKEY_SET_POT_PARM1_COMMAND");
#endif //DEBUG_WINKEY
            winkey_status = WINKEY_SET_POT_PARM1_COMMAND;
            break;
          case 0x06:
            winkey_status = WINKEY_PAUSE_COMMAND;
#ifdef DEBUG_WINKEY
            displayCache.add("service_winkey:WINKEY_PAUSE_COMMAND");
#endif //DEBUG_WINKEY
            break;
          case 0x07:
#ifdef FEATURE_POTENTIOMETER
            //REL winkey_port_write(((pot_value_wpm()-pot_wpm_low_value)|128),0);
#else
          winkey_port_write((byte(configuration.wpm - pot_wpm_low_value) | 128), 0);
#endif
#ifdef DEBUG_WINKEY
            displayCache.add("service_winkey:report pot");
#endif //DEBUG_WINKEY
            break;
          case 0x08: // backspace command
            if (send_buffer_bytes)
            {
              send_buffer_bytes--;
            }
#ifdef DEBUG_WINKEY
            displayCache.add("service_winkey:backspace");
#endif //DEBUG_WINKEY
            break;
          case 0x09:
#ifdef DEBUG_WINKEY
            displayCache.add("service_winkey:WINKEY_SET_PINCONFIG_COMMAND");
#endif //DEBUG_WINKEY
            winkey_status = WINKEY_SET_PINCONFIG_COMMAND;
            break;
          case 0x0a: // 0A - clear buffer - no parms
            // #ifdef DEBUG_WINKEY
            //   displayCache.add("service_winkey:0A clear buff");
            // #endif //DEBUG_WINKEY
            //REL clear_send_buffer();
            if (winkey_sending)
            {
              //clear_send_buffer();
              winkey_sending = 0;
              displayCache.add(" SENDING FROM CLEAR BUFFER ");
              winkey_port_write(0xc0 | winkey_sending | winkey_xoff, 0);
            }
            pause_sending_buffer = 0;
            winkey_buffer_counter = 0;
            winkey_buffer_pointer = 0;
#if !defined(OPTION_DISABLE_SERIAL_PORT_CHECKING_WHILE_SENDING_CW)
            //REL loop_element_lengths_breakout_flag = 0;
#endif
#ifdef FEATURE_MEMORIES
            repeat_memory = 255;
#endif
            sending_mode = AUTOMATIC_SENDING;
            manual_ptt_invoke = 0;
            //REL tx_and_sidetone_key(0);
            winkey_speed_state = WINKEY_UNBUFFERED_SPEED;
//REL configuration.wpm = winkey_last_unbuffered_speed_wpm;
#ifdef DEBUG_WINKEY
            displayCache.add("service_winkey:0A clearbuff send_buffer_bytes:");
            displayCache.add(String(send_buffer_bytes));
#endif //DEBUG_WINKEY
            break;
          case 0x0b:
            winkey_status = WINKEY_KEY_COMMAND;
#ifdef DEBUG_WINKEY
            displayCache.add("service_winkey:WINKEY_KEY_COMMAND");
#endif //DEBUG_WINKEY
            break;
          case 0x0c:
            winkey_status = WINKEY_HSCW_COMMAND;
#ifdef DEBUG_WINKEY
            displayCache.add("service_winkey:WINKEY_HSCW_COMMAND");
#endif //DEBUG_WINKEY
            break;
          case 0x0d:
            winkey_status = WINKEY_FARNSWORTH_COMMAND;
#ifdef DEBUG_WINKEY
            displayCache.add("service_winkey:WINKEY_FARNSWORTH_COMMAND");
#endif //DEBUG_WINKEY
            break;
          case 0x0e:
            winkey_status = WINKEY_SETMODE_COMMAND;
#ifdef DEBUG_WINKEY
            displayCache.add("service_winkey:WINKEY_SETMODE_COMMAND");
#endif //DEBUG_WINKEY
            break;
          case 0x0f: // bulk load of defaults
            winkey_status = WINKEY_LOAD_SETTINGS_PARM_1_COMMAND;
#ifdef DEBUG_WINKEY
            displayCache.add("service_winkey:WINKEY_LOAD_SETTINGS_PARM_1_COMMAND");
#endif //DEBUG_WINKEY
            break;
          case 0x10:
            winkey_status = WINKEY_FIRST_EXTENSION_COMMAND;
#ifdef DEBUG_WINKEY
            displayCache.add("service_winkey:WINKEY_FIRST_EXTENSION_COMMAND");
#endif //DEBUG_WINKEY
            break;
          case 0x11:
            winkey_status = WINKEY_KEYING_COMPENSATION_COMMAND;
#ifdef DEBUG_WINKEY
            displayCache.add("service_winkey:WINKEY_KEYING_COMPENSATION_COMMAND");
#endif //DEBUG_WINKEY
            break;
          case 0x12:
            winkey_status = WINKEY_UNSUPPORTED_COMMAND;
#ifdef DEBUG_WINKEY
            displayCache.add("service_winkey:0x12unsupport");
#endif //DEBUG_WINKEY
            winkey_parmcount = 1;
            break;
          case 0x13: // NULL command
#ifdef DEBUG_WINKEY
            displayCache.add("service_winkey:0x13null");
#endif //DEBUG_WINKEY
            break;
          case 0x14:
            winkey_status = WINKEY_SOFTWARE_PADDLE_COMMAND;
#ifdef DEBUG_WINKEY
            displayCache.add("service_winkey:WINKEY_SOFTWARE_PADDLE_COMMAND");
#endif //DEBUG_WINKEY
            break;
          case 0x15:                              // report status
#ifndef OPTION_WINKEY_IGNORE_FIRST_STATUS_REQUEST //--------------------
            status_byte_to_send = 0xc0 | winkey_sending | winkey_xoff;
            if (send_buffer_status == SERIAL_SEND_BUFFER_TIMED_COMMAND)
            {
              status_byte_to_send = status_byte_to_send | 16;
            }
            displayCache.add(" reporting status ");
            winkey_port_write(status_byte_to_send, 0);
#ifdef DEBUG_WINKEY
            displayCache.add("service_winkey:0x15 rpt status:");
            displayCache.add(String(status_byte_to_send));
#endif //DEBUG_WINKEY
#else  //OPTION_WINKEY_IGNORE_FIRST_STATUS_REQUEST ------------------------
          if (ignored_first_status_request)
          {
            status_byte_to_send = 0xc0 | winkey_sending | winkey_xoff;
            if (send_buffer_status == SERIAL_SEND_BUFFER_TIMED_COMMAND)
            {
              status_byte_to_send = status_byte_to_send | 16;
            }
            winkey_port_write(status_byte_to_send, 0);
#ifdef DEBUG_WINKEY
            displayCache.add("service_winkey:0x15 rpt status:");
            displayCache.add(status_byte_to_send);
#endif //DEBUG_WINKEY
          }
          else
          {
            ignored_first_status_request = 1;
#ifdef DEBUG_WINKEY
            displayCache.add("service_winkey:ignored first 0x15 status request");
#endif //DEBUG_WINKEY
          }
#endif //OPTION_WINKEY_IGNORE_FIRST_STATUS_REQUEST --------------------

            break;
          case 0x16: // Pointer operation
            winkey_status = WINKEY_POINTER_COMMAND;
#ifdef DEBUG_WINKEY
            displayCache.add("service_winkey:WINKEY_POINTER_COMMAND");
#endif //DEBUG_WINKEY
            break;
          case 0x17: // dit to dah ratio
            winkey_status = WINKEY_DAH_TO_DIT_RATIO_COMMAND;
#ifdef DEBUG_WINKEY
            displayCache.add("service_winkey:WINKEY_DAH_TO_DIT_RATIO_COMMAND");
#endif //DEBUG_WINKEY
            break;
          // start of buffered commands ------------------------------
          case 0x18: //buffer PTT on/off
            winkey_status = WINKEY_BUFFFERED_PTT_COMMMAND;
#ifdef DEBUG_WINKEY
            displayCache.add("service_winkey:WINKEY_BUFFFERED_PTT_COMMMAND");
#endif //DEBUG_WINKEY
            break;
          case 0x19:
            winkey_status = WINKEY_KEY_BUFFERED_COMMAND;
#ifdef DEBUG_WINKEY
            displayCache.add("service_winkey:WINKEY_KEY_BUFFERED_COMMAND");
#endif //DEBUG_WINKEY
            break;
          case 0x1a:
            winkey_status = WINKEY_WAIT_BUFFERED_COMMAND;
#ifdef DEBUG_WINKEY
            displayCache.add("service_winkey:WINKEY_WAIT_BUFFERED_COMMAND");
#endif //DEBUG_WINKEY
            break;
          case 0x1b:
            winkey_status = WINKEY_MERGE_COMMAND;
#ifdef DEBUG_WINKEY
            displayCache.add("service_winkey:WINKEY_MERGE_COMMAND");
#endif //DEBUG_WINKEY
            break;
          case 0x1c: // speed command - buffered
            winkey_status = WINKEY_BUFFERED_SPEED_COMMAND;
#ifdef DEBUG_WINKEY
            displayCache.add("service_winkey:WINKEY_BUFFERED_SPEED_COMMAND");
#endif //DEBUG_WINKEY
            break;
          case 0x1d:
            winkey_status = WINKEY_BUFFERED_HSCW_COMMAND;
#ifdef DEBUG_WINKEY
            displayCache.add("service_winkey:WINKEY_BUFFERED_HSCW_COMMAND");
#endif //DEBUG_WINKEY
            break;
          case 0x1e: // cancel buffered speed command - buffered

#ifdef DEBUG_WINKEY
            displayCache.add("service_winkey:WINKEY_CANCEL_BUFFERED_SPEED_COMMAND");
#endif //DEBUG_WINKEY

            if (winkey_speed_state == WINKEY_BUFFERED_SPEED)
            {
              //REL add_to_send_buffer(SERIAL_SEND_BUFFER_WPM_CHANGE);
              //REL add_to_send_buffer(0);
              //REL add_to_send_buffer((byte)winkey_last_unbuffered_speed_wpm);
              winkey_speed_state = WINKEY_UNBUFFERED_SPEED;
#ifdef DEBUG_WINKEY
              displayCache.add("service_winkey:winkey_speed_state = WINKEY_UNBUFFERED_SPEED");
#endif //DEBUG_WINKEY
            }
            else
            {
#ifdef DEBUG_WINKEY
              displayCache.add("service_winkey:WINKEY_CANCEL_BUFFERED_SPEED_COMMAND: no action");
#endif //DEBUG_WINKEY
            }
            winkey_status = WINKEY_NO_COMMAND_IN_PROGRESS;
            break;
          case 0x1f: // buffered NOP - no need to do anything
#ifdef DEBUG_WINKEY
            displayCache.add("service_winkey:1F NOP");
#endif //DEBUG_WINKEY
            break;

#ifdef OPTION_WINKEY_EXTENDED_COMMANDS_WITH_DOLLAR_SIGNS
          case 36:
            winkey_status = WINKEY_EXTENDED_COMMAND;
            beep();
#ifdef DEBUG_WINKEY
            displayCache.add("service_winkey:WINKEY_EXTENDED_COMMAND");
#endif //DEBUG_WINKEY
            break;
#endif      //OPTION_WINKEY_EXTENDED_COMMANDS_WITH_DOLLAR_SIGNS
          } //switch (incoming_serial_byte)

#ifdef OPTION_WINKEY_STRICT_HOST_OPEN
        } //if ((winkey_host_open) || (incoming_serial_byte == 0))
#endif
      }
    }
    else
    { //if (winkey_status == WINKEY_NO_COMMAND_IN_PROGRESS)

      if (winkey_status == WINKEY_UNSUPPORTED_COMMAND)
      {
        winkey_parmcount--;
#ifdef DEBUG_WINKEY
        displayCache.add("service_winkey:WINKEY_UNSUPPORTED_COMMAND winkey_parmcount:");
        displayCache.add(String(winkey_parmcount));
#endif //DEBUG_WINKEY
        if (winkey_parmcount == 0)
        {
          displayCache.add(" SENDING PARMCOUNT ");

          winkey_port_write(0xc0 | winkey_sending | winkey_xoff, 0);
          winkey_status = WINKEY_NO_COMMAND_IN_PROGRESS;
#ifdef DEBUG_WINKEY
          displayCache.add("service_winkey:WINKEY_UNSUPPORTED_COMMAND:WINKEY_NO_COMMAND_IN_PROGRESS");
          displayCache.add(String(winkey_parmcount));
#endif //DEBUG_WINKEY
        }
      }

      //WINKEY_LOAD_SETTINGS_PARM_1_COMMAND IS 101
      if ((winkey_status > 100) && (winkey_status < 116))
      { // Load Settings Command - this has 15 parameters, so we handle it a bit differently
        //REL winkey_load_settings_command(winkey_status,incoming_serial_byte);
        winkey_status++;
        if (winkey_status > 115)
        {
          winkey_status = WINKEY_NO_COMMAND_IN_PROGRESS;
#ifdef DEBUG_WINKEY
          displayCache.add("service_winkey:WINKEY_LOAD_SETTINGS_PARM_15->NOCMD");
#endif //DEBUG_WINKEY
        }
      }

#ifdef OPTION_WINKEY_EXTENDED_COMMANDS_WITH_DOLLAR_SIGNS
      if (winkey_status == WINKEY_EXTENDED_COMMAND)
      {
        //if (incoming_serial_byte != 36){
        //beep();
        //} else {
        boop();
        beep();
        winkey_status = WINKEY_NO_COMMAND_IN_PROGRESS;
        //}
      }
#endif //OPTION_WINKEY_EXTENDED_COMMANDS_WITH_DOLLAR_SIGNS

      if (winkey_status == WINKEY_SET_PINCONFIG_COMMAND)
      {
        //REL winkey_set_pinconfig_command(incoming_serial_byte);
        winkey_status = WINKEY_NO_COMMAND_IN_PROGRESS;
      }

      if (winkey_status == WINKEY_MERGE_COMMAND)
      {
#ifdef FEATURE_MEMORIES
        repeat_memory = 255;
#endif
        //REL add_to_send_buffer(SERIAL_SEND_BUFFER_PROSIGN);
        //REL add_to_send_buffer(incoming_serial_byte);
        winkey_status = WINKEY_MERGE_PARM_2_COMMAND;
      }
      else
      {
        if (winkey_status == WINKEY_MERGE_PARM_2_COMMAND)
        {
          //REL add_to_send_buffer(incoming_serial_byte);
          winkey_status = WINKEY_NO_COMMAND_IN_PROGRESS;
        }
      }

      if (winkey_status == WINKEY_UNBUFFERED_SPEED_COMMAND)
      {
        //REL winkey_unbuffered_speed_command(incoming_serial_byte);
        displayCache.add(" SETTING NO CMD AFTER UNBUF SPEED ");
        winkey_status = WINKEY_NO_COMMAND_IN_PROGRESS;
      }

      if (winkey_status == WINKEY_FARNSWORTH_COMMAND)
      {
        //REL winkey_farnsworth_command(incoming_serial_byte);
        winkey_status = WINKEY_NO_COMMAND_IN_PROGRESS;
      }

      if (winkey_status == WINKEY_HSCW_COMMAND)
      {
        if (incoming_serial_byte == 0)
        {
#ifdef FEATURE_POTENTIOMETER
          //REL configuration.pot_activated = 1;
#endif
        }
        else
        {
//REL configuration.wpm = ((incoming_serial_byte*100)/5);
//REL winkey_last_unbuffered_speed_wpm = configuration.wpm;
#ifdef OPTION_WINKEY_STRICT_EEPROM_WRITES_MAY_WEAR_OUT_EEPROM
          config_dirty = 1;
#endif
        }
        winkey_status = WINKEY_NO_COMMAND_IN_PROGRESS;
      }

      if (winkey_status == WINKEY_BUFFERED_SPEED_COMMAND)
      {
#ifdef DEBUG_WINKEY
        displayCache.add("service_winkey:BUFFERED_SPEED_CMD:send_buffer_bytes:");
        displayCache.add(String(send_buffer_bytes));
#endif //DEBUG_WINKEY
//REL add_to_send_buffer(SERIAL_SEND_BUFFER_WPM_CHANGE);
//REL add_to_send_buffer(0);
//REL add_to_send_buffer(incoming_serial_byte);
#ifdef DEBUG_WINKEY
        displayCache.add("service_winkey:BUFFERED_SPEED_CMD:send_buffer_bytes:");
        displayCache.add(String(send_buffer_bytes));
#endif //DEBUG_WINKEY
#ifdef DEBUG_WINKEY
        displayCache.add("service_winkey:WINKEY_BUFFERED_SPEED_COMMAND->NOCMD");
#endif //DEBUG_WINKEY
        winkey_status = WINKEY_NO_COMMAND_IN_PROGRESS;
      }

      if (winkey_status == WINKEY_BUFFERED_HSCW_COMMAND)
      {
        if (incoming_serial_byte > 1)
        { // the HSCW command is overloaded; 0 = buffered TX 1, 1 = buffered TX 2, > 1 = HSCW WPM
          unsigned int send_buffer_wpm = ((incoming_serial_byte * 100) / 5);
          //REL add_to_send_buffer(SERIAL_SEND_BUFFER_WPM_CHANGE);
          //REL add_to_send_buffer(highByte(send_buffer_wpm));
          //REL add_to_send_buffer(lowByte(send_buffer_wpm));
        }
        else
        {
          //REL add_to_send_buffer(SERIAL_SEND_BUFFER_TX_CHANGE);
          //REL add_to_send_buffer(incoming_serial_byte+1);
        }
        winkey_status = WINKEY_NO_COMMAND_IN_PROGRESS;
      }

      if (winkey_status == WINKEY_KEY_BUFFERED_COMMAND)
      {
        //REL add_to_send_buffer(SERIAL_SEND_BUFFER_TIMED_KEY_DOWN);
        //REL add_to_send_buffer(incoming_serial_byte);
        winkey_status = WINKEY_NO_COMMAND_IN_PROGRESS;
      }

      if (winkey_status == WINKEY_WAIT_BUFFERED_COMMAND)
      {
        //REL add_to_send_buffer(SERIAL_SEND_BUFFER_TIMED_WAIT);
        //REL add_to_send_buffer(incoming_serial_byte);
        winkey_status = WINKEY_NO_COMMAND_IN_PROGRESS;
      }

      if (winkey_status == WINKEY_BUFFFERED_PTT_COMMMAND)
      {
        if (incoming_serial_byte)
        {
          //REL add_to_send_buffer(SERIAL_SEND_BUFFER_PTT_ON);
        }
        else
        {
          //REL add_to_send_buffer(SERIAL_SEND_BUFFER_PTT_OFF);
        }
        winkey_status = WINKEY_NO_COMMAND_IN_PROGRESS;
      }

      if (winkey_status == WINKEY_POINTER_01_COMMAND)
      { // move input pointer to new positon in overwrite mode
        winkey_buffer_pointer = incoming_serial_byte;
        winkey_status = WINKEY_NO_COMMAND_IN_PROGRESS;
#ifdef DEBUG_WINKEY
        displayCache.add("service_winkey:PTR1_CMD->NOCMD winkey_buffer_pointer:");
        displayCache.add(String(winkey_buffer_pointer));
        displayCache.add("send_buffer_bytes:");
        displayCache.add(String(send_buffer_bytes));
#endif //DEBUG_WINKEY
      }

      if (winkey_status == WINKEY_POINTER_02_COMMAND)
      { // move input pointer to new position in append mode
        winkey_buffer_pointer = 0;
        winkey_status = WINKEY_NO_COMMAND_IN_PROGRESS;
#ifdef DEBUG_WINKEY
        displayCache.add("service_winkey:PTR2_CMD->NOCMD send_buffer_bytes:");
        displayCache.add(String(send_buffer_bytes));
        displayCache.add(" winkey_buffer_counter:");
        displayCache.add(String(winkey_buffer_counter));
        displayCache.add(" winkey_buffer_pointer:");
        displayCache.add(String(winkey_buffer_pointer));
#endif //DEBUG_WINKEY
      }

      if (winkey_status == WINKEY_POINTER_03_COMMAND)
      { // add multiple nulls to buffer
#ifdef DEBUG_WINKEY
        displayCache.add("service_winkey:PTR3_CMD send_buffer_bytes:");
        displayCache.add(String(send_buffer_bytes));
        displayCache.add(" winkey_buffer_counter:");
        displayCache.add(String(winkey_buffer_counter));
        displayCache.add(" winkey_buffer_pointer:");
        displayCache.add(String(winkey_buffer_pointer));
#endif //DEBUG_WINKEY
        byte serial_buffer_position_to_overwrite;
        for (byte x = incoming_serial_byte; x > 0; x--)
        {
          if (winkey_buffer_pointer > 0)
          {
            serial_buffer_position_to_overwrite = send_buffer_bytes - (winkey_buffer_counter - winkey_buffer_pointer) - 1;
            if ((send_buffer_bytes) && (serial_buffer_position_to_overwrite < send_buffer_bytes))
            {
              send_buffer_array[serial_buffer_position_to_overwrite] = SERIAL_SEND_BUFFER_NULL;
            }
            winkey_buffer_pointer++;
          }
          else
          {
            //REL add_to_send_buffer(SERIAL_SEND_BUFFER_NULL);
            winkey_buffer_counter++;
          }
        }
        winkey_status = WINKEY_NO_COMMAND_IN_PROGRESS;
#ifdef DEBUG_WINKEY
        displayCache.add("service_winkey:PTR3_CMD->NO_CMD send_buffer_bytes:");
        displayCache.add(String(send_buffer_bytes));
        displayCache.add(" winkey_buffer_counter:");
        displayCache.add(String(winkey_buffer_counter));
        displayCache.add(" winkey_buffer_pointer:");
        displayCache.add(String(winkey_buffer_pointer));
#endif //DEBUG_WINKEY
      }

      if (winkey_status == WINKEY_POINTER_COMMAND)
      {
        switch (incoming_serial_byte)
        {
        case 0x00:
          winkey_buffer_counter = 0;
          winkey_buffer_pointer = 0;
          winkey_status = WINKEY_NO_COMMAND_IN_PROGRESS;
#ifdef DEBUG_WINKEY
          displayCache.add("service_winkey:PTR_CMD->NOCMD send_buffer_bytes:");
          displayCache.add(String(send_buffer_bytes));
          displayCache.add(" winkey_buffer_counter:");
          displayCache.add(String(winkey_buffer_counter));
          displayCache.add(" winkey_buffer_pointer:");
          displayCache.add(String(winkey_buffer_pointer));
#endif //DEBUG_WINKEY
          break;
        case 0x01:
          winkey_status = WINKEY_POINTER_01_COMMAND;
#ifdef DEBUG_WINKEY
          displayCache.add("service_winkey:PTR1_CMD");
#endif //DEBUG_WINKEY
          break;
        case 0x02:
          winkey_status = WINKEY_POINTER_02_COMMAND; // move to new position in append mode
#ifdef DEBUG_WINKEY
          displayCache.add("service_winkey:PTR2_CMD");
#endif //DEBUG_WINKEY
          break;
        case 0x03:
          winkey_status = WINKEY_POINTER_03_COMMAND;
#ifdef DEBUG_WINKEY
          displayCache.add("service_winkey:PTR3_CMD");
#endif //DEBUG_WINKEY
          break;
        default:
          winkey_status = WINKEY_NO_COMMAND_IN_PROGRESS;
#ifdef DEBUG_WINKEY
          displayCache.add("service_winkey:PTR_CMD->NOCMD");
#endif //DEBUG_WINKEY
          break;
        }
      }

#ifdef OPTION_WINKEY_2_SUPPORT
      if (winkey_status == WINKEY_SEND_MSG)
      {
        if ((incoming_serial_byte > 0) && (incoming_serial_byte < 7))
        {
//REL add_to_send_buffer(SERIAL_SEND_BUFFER_MEMORY_NUMBER);
//REL add_to_send_buffer(incoming_serial_byte - 1);
#ifdef FEATURE_MEMORIES
          repeat_memory = 255;
#endif
        }
        winkey_status = WINKEY_NO_COMMAND_IN_PROGRESS;
      }
#endif //OPTION_WINKEY_2_SUPPORT

      if (winkey_status == WINKEY_ADMIN_COMMAND)
      {
        switch (incoming_serial_byte)
        {
        case 0x00:
          winkey_status = WINKEY_UNSUPPORTED_COMMAND;
          winkey_parmcount = 1;
#ifdef DEBUG_WINKEY
          displayCache.add("service_winkey:calib cmd UNSUPPORTED_COMMAND await 1 parm");
#endif           //DEBUG_WINKEY
          break; // calibrate command
        case 0x01:
#ifdef DEBUG_WINKEY
          displayCache.add("service_winkey:WINKEY_ADMIN_COMMAND 0x01");
#endif                           //DEBUG_WINKEY
#if defined(__AVR__)             //#ifndef ARDUINO_SAM_DUE
          asm volatile("jmp 0"); /*wdt_enable(WDTO_30MS); while(1) {};*/
#else
          setup();
#endif             //__AVR__
          break;   // reset command
        case 0x02: // host open command - send version back to host
#ifdef OPTION_WINKEY_2_SUPPORT
          displayCache.add(" SENDING VERSION NUMBER ");
          winkey_port_write(WINKEY_2_REPORT_VERSION_NUMBER, 1);
#else  //OPTION_WINKEY_2_SUPPORT
          winkey_port_write(WINKEY_1_REPORT_VERSION_NUMBER, 1);
#endif //OPTION_WINKEY_2_SUPPORT
          winkey_status = WINKEY_NO_COMMAND_IN_PROGRESS;
          manual_ptt_invoke = 0;
          winkey_host_open = 1;
#ifdef DEBUG_WINKEY
          displayCache.add("service_winkey:ADMIN_CMD hostopen");
#endif //DEBUG_WINKEY
#if defined(OPTION_WINKEY_BLINK_PTT_ON_HOST_OPEN)
          blink_ptt_dits_and_dahs("..");
#else
          //REL boop_beep();
#endif
          break;
        case 0x03: // host close command
          winkey_status = WINKEY_NO_COMMAND_IN_PROGRESS;
          manual_ptt_invoke = 0;
          winkey_host_open = 0;
#ifdef OPTION_WINKEY_SEND_VERSION_ON_HOST_CLOSE
#ifdef OPTION_WINKEY_2_SUPPORT
          winkey_port_write(WINKEY_2_REPORT_VERSION_NUMBER, 1);
#else  //OPTION_WINKEY_2_SUPPORT
          winkey_port_write(WINKEY_1_REPORT_VERSION_NUMBER, 1);
#endif //OPTION_WINKEY_2_SUPPORT
#endif //OPTION_WINKEY_SEND_VERSION_ON_HOST_CLOSE
#ifdef DEBUG_WINKEY
          displayCache.add("service_winkey:ADMIN_CMD hostclose");
#endif //DEBUG_WINKEY
//REL beep_boop();
//REL config_dirty = 1;
#if defined(OPTION_WINKEY_2_SUPPORT) && !defined(OPTION_WINKEY_2_HOST_CLOSE_NO_SERIAL_PORT_RESET)
          primary_serial_port->end();
          primary_serial_port->begin(1200);
#endif
          break;
        case 0x04: // echo command
          winkey_status = WINKEY_ADMIN_COMMAND_ECHO;
#ifdef DEBUG_WINKEY
          displayCache.add("service_winkey:ADMIN_CMD_ECHO");
#endif //DEBUG_WINKEY
          break;
        case 0x05: // paddle A2D
          winkey_port_write(WINKEY_RETURN_THIS_FOR_ADMIN_PADDLE_A2D, 0);
          winkey_status = WINKEY_NO_COMMAND_IN_PROGRESS;
#ifdef DEBUG_WINKEY
          displayCache.add("service_winkey:ADMIN_CMD paddleA2D");
#endif //DEBUG_WINKEY
          break;
        case 0x06: // speed A2D
          winkey_port_write(WINKEY_RETURN_THIS_FOR_ADMIN_SPEED_A2D, 0);
          winkey_status = WINKEY_NO_COMMAND_IN_PROGRESS;
#ifdef DEBUG_WINKEY
          displayCache.add("service_winkey:ADMIN_CMD speedA2D");
#endif //DEBUG_WINKEY
          break;
        case 0x07: // Get values
#ifdef DEBUG_WINKEY
          displayCache.add("service_winkey:ADMIN_CMD winkey_admin_get_values");
#endif //DEBUG_WINKEY \
    //REL winkey_admin_get_values_command();
          winkey_status = WINKEY_NO_COMMAND_IN_PROGRESS;
          break;
        case 0x08: // reserved
#ifdef DEBUG_WINKEY
          displayCache.add("service_winkey:ADMIN_CMD0x08reserved-WTF");
#endif //DEBUG_WINKEY
          winkey_status = WINKEY_NO_COMMAND_IN_PROGRESS;
          break;
        case 0x09: // get cal
#ifdef DEBUG_WINKEY
          displayCache.add("service_winkey:ADMIN_CMDgetcal");
#endif //DEBUG_WINKEY
          winkey_port_write(WINKEY_RETURN_THIS_FOR_ADMIN_GET_CAL, 0);
          winkey_status = WINKEY_NO_COMMAND_IN_PROGRESS;
          break;
#ifdef OPTION_WINKEY_2_SUPPORT
        case 0x0a: // set wk1 mode (10)
          wk2_mode = 1;
          winkey_status = WINKEY_NO_COMMAND_IN_PROGRESS;
#ifdef DEBUG_WINKEY
          displayCache.add("service_winkey:ADMIN_CMD wk2_mode1");
#endif //DEBUG_WINKEY
          break;
        case 0x0b: // set wk2 mode (11)
#ifdef DEBUG_WINKEY
          displayCache.add("service_winkey:ADMIN_CMD wk2_mode2");
#endif //DEBUG_WINKEY \
    //REL beep();     \
    //REL beep();
          wk2_mode = 2;
          winkey_status = WINKEY_NO_COMMAND_IN_PROGRESS;
          break;
        case 0x0c: // download EEPPROM 256 bytes (12)
#ifdef DEBUG_WINKEY
          displayCache.add("service_winkey:ADMIN_CMD winkey_eeprom_download");
#endif //DEBUG_WINKEY \
    //REL winkey_eeprom_download();
          winkey_status = WINKEY_NO_COMMAND_IN_PROGRESS;
          break;
        case 0x0d: // upload EEPROM 256 bytes (13)
#ifdef DEBUG_WINKEY
          displayCache.add("service_winkey:ADMIN_CMD uploadEEPROM");
#endif                                                //DEBUG_WINKEY
          winkey_status = WINKEY_UNSUPPORTED_COMMAND; // upload EEPROM 256 bytes
          winkey_parmcount = 256;
          break;
        case 0x0e: //(14)
#ifdef DEBUG_WINKEY
          displayCache.add("service_winkey:ADMIN_CMD WINKEY_SEND_MSG");
#endif //DEBUG_WINKEY
          winkey_status = WINKEY_SEND_MSG;
          break;
        case 0x0f: // load xmode (15)
          winkey_status = WINKEY_UNSUPPORTED_COMMAND;
          winkey_parmcount = 1;
#ifdef DEBUG_WINKEY
          displayCache.add("service_winkey:ADMIN_CMD loadxmode");
#endif //DEBUG_WINKEY
          break;
        case 0x10: // reserved (16)
          winkey_port_write(zero, 0);
          winkey_status = WINKEY_NO_COMMAND_IN_PROGRESS;
          break;
        case 0x11: // set high baud rate (17)
#ifdef DEBUG_WINKEY
          displayCache.add("service_winkey:ADMIN_CMD sethighbaudrate");
#endif //DEBUG_WINKEY                 \
    //REL primary_serial_port->end(); \
    //REL primary_serial_port->begin(9600);
          Serial.end();
          Serial.begin(9600);
          winkey_status = WINKEY_NO_COMMAND_IN_PROGRESS;
          break;
        case 0x12: // set low baud rate (18)
#ifdef DEBUG_WINKEY
          displayCache.add("service_winkey:ADMIN_CMD setlowbaudrate");
#endif //DEBUG_WINKEY                 \
    //REL primary_serial_port->end(); \
    //REL primary_serial_port->begin(1200);
          Serial.end();
          Serial.begin(1200);
          winkey_status = WINKEY_NO_COMMAND_IN_PROGRESS;
          break;
#endif //OPTION_WINKEY_2_SUPPORT

#ifdef FEATURE_SO2R_BASE
        case 0xF0: // Send SO2R device information
#ifdef DEBUG_WINKEY
          diplayCache.add("service_winkey:ADMIN_CMD getSO2Rdeviceinfo");
#endif

          static const uint8_t device_info[] = {
              0xAA, 0x55, 0xCC, 0x33, // Header
              1, 0, 0,                // SO2R Major, minor, patch
              1, 0,                   // Protocol major, minor
              0,                      // capabilities - bit 0 is stereo reverse, others undefined
          };

          for (uint8_t i = 0; i < sizeof(device_info); i++)
          {
            winkey_port_write(device_info[i], 1);
          }

          static const byte *name = (byte *)SO2R_DEVICE_NAME;
          for (uint8_t i = 0; i < sizeof(SO2R_DEVICE_NAME); i++)
          {
            winkey_port_write(name[i], 1);
          }

          static const byte *version = (byte *)CODE_VERSION;
          for (uint8_t i = 0; i < sizeof(CODE_VERSION); i++)
          {
            winkey_port_write(version[i], 1);
          }
          winkey_status = WINKEY_NO_COMMAND_IN_PROGRESS;
          break;

        case 0xFF: // No-op
          diplayCache.add("service_winkey:NO-OP");
          winkey_status = WINKEY_NO_COMMAND_IN_PROGRESS;
          break;

#endif //FEATURE_SO2R_BASE

        default:
          winkey_status = WINKEY_NO_COMMAND_IN_PROGRESS;
#ifdef DEBUG_WINKEY
          displayCache.add("service_winkey:ADMIN_CMD->NOCMD");
#endif //DEBUG_WINKEY
          break;
        }
      }
      else
      {
        if (winkey_status == WINKEY_ADMIN_COMMAND_ECHO)
        {
#ifdef DEBUG_WINKEY
          displayCache.add("service_winkey:ADMIN_CMD echoabyte.");
#endif //DEBUG_WINKEY
          winkey_port_write(incoming_serial_byte, 1);
          winkey_status = WINKEY_NO_COMMAND_IN_PROGRESS;
        }
      }

      if (winkey_status == WINKEY_KEYING_COMPENSATION_COMMAND)
      {
#ifdef DEBUG_WINKEY
        displayCache.add("service_winkey:ADMIN_CMD WINKEY_KEYING_COMPENSATION_COMMAND byte");
#endif //DEBUG_WINKEY \
    //REL configuration.keying_compensation = incoming_serial_byte;
        winkey_status = WINKEY_NO_COMMAND_IN_PROGRESS;
      }

      if (winkey_status == WINKEY_FIRST_EXTENSION_COMMAND)
      {
#ifdef DEBUG_WINKEY
        displayCache.add("service_winkey:ADMIN_COMMAND WINKEY_FIRST_EXTENSION_COMMAND byte");
#endif //DEBUG_WINKEY
        first_extension_time = incoming_serial_byte;
#ifdef DEBUG_WINKEY_PROTOCOL_USING_CW
        send_char('X', KEYER_NORMAL);
#endif
        winkey_status = WINKEY_NO_COMMAND_IN_PROGRESS;
      }

      if (winkey_status == WINKEY_PAUSE_COMMAND)
      {
        if (incoming_serial_byte)
        {
#ifdef DEBUG_WINKEY
          displayCache.add("service_winkey:ADMIN_CMD WINKEY_PAUSE_COMMANDpause");
#endif //DEBUG_WINKEY
          pause_sending_buffer = 1;
        }
        else
        {
#ifdef DEBUG_WINKEY
          displayCache.add("service_winkey:ADMIN_CMD WINKEY_PAUSE_COMMANDunpause");
#endif //DEBUG_WINKEY
          pause_sending_buffer = 0;
        }
        winkey_status = WINKEY_NO_COMMAND_IN_PROGRESS;
      }

      if (winkey_status == WINKEY_KEY_COMMAND)
      {
#ifdef FEATURE_MEMORIES
        repeat_memory = 255;
#endif
        sending_mode = AUTOMATIC_SENDING;
        if (incoming_serial_byte)
        {
          //REL tx_and_sidetone_key(1);
        }
        else
        {
          //REL tx_and_sidetone_key(0);
        }
        winkey_status = WINKEY_NO_COMMAND_IN_PROGRESS;
      }

      if (winkey_status == WINKEY_DAH_TO_DIT_RATIO_COMMAND)
      {
        //REL winkey_dah_to_dit_ratio_command(incoming_serial_byte);
        winkey_status = WINKEY_NO_COMMAND_IN_PROGRESS;
      }

      if (winkey_status == WINKEY_WEIGHTING_COMMAND)
      {
        //REL winkey_weighting_command(incoming_serial_byte);
        winkey_status = WINKEY_NO_COMMAND_IN_PROGRESS;
      }

      if (winkey_status == WINKEY_PTT_TIMES_PARM1_COMMAND)
      {
        //REL winkey_ptt_times_parm1_command(incoming_serial_byte);
        winkey_status = WINKEY_PTT_TIMES_PARM2_COMMAND;
      }
      else
      {
        if (winkey_status == WINKEY_PTT_TIMES_PARM2_COMMAND)
        {
          //REL winkey_ptt_times_parm2_command(incoming_serial_byte);
          winkey_status = WINKEY_NO_COMMAND_IN_PROGRESS;
        }
      }

      if (winkey_status == WINKEY_SET_POT_PARM1_COMMAND)
      {
        //REL winkey_set_pot_parm1_command(incoming_serial_byte);
        winkey_status = WINKEY_SET_POT_PARM2_COMMAND;
      }
      else
      {
        if (winkey_status == WINKEY_SET_POT_PARM2_COMMAND)
        {
          //REL winkey_set_pot_parm2_command(incoming_serial_byte);
          winkey_status = WINKEY_SET_POT_PARM3_COMMAND;
        }
        else
        {
          if (winkey_status == WINKEY_SET_POT_PARM3_COMMAND)
          { // third parm is max read value from pot, depending on wiring
            //REL winkey_set_pot_parm3_command(incoming_serial_byte); // WK2 protocol just ignores this third parm
            winkey_status = WINKEY_NO_COMMAND_IN_PROGRESS; // this is taken care of in winkey_set_pot_parm3()
          }
        }
      }

      if (winkey_status == WINKEY_SETMODE_COMMAND)
      {
        //REL winkey_setmode_command(incoming_serial_byte);
        winkey_status = WINKEY_NO_COMMAND_IN_PROGRESS;
      }

      if (winkey_status == WINKEY_SOFTWARE_PADDLE_COMMAND)
      {
#ifdef FEATURE_MEMORIES
        repeat_memory = 255;
#endif
        switch (incoming_serial_byte)
        {
        case 0:
          winkey_dit_invoke = 0;
          winkey_dah_invoke = 0;
          break;
        case 1:
          winkey_dit_invoke = 0;
          winkey_dah_invoke = 1;
          break;
        case 2:
          winkey_dit_invoke = 1;
          winkey_dah_invoke = 0;
          break;
        case 3:
          winkey_dah_invoke = 1;
          winkey_dit_invoke = 1;
          break;
        }
        winkey_status = WINKEY_NO_COMMAND_IN_PROGRESS;
      }

      if (winkey_status == WINKEY_SIDETONE_FREQ_COMMAND)
      {
        //REL winkey_sidetone_freq_command(incoming_serial_byte);
        winkey_status = WINKEY_NO_COMMAND_IN_PROGRESS;
      }

    } // else (winkey_status == WINKEY_NO_COMMAND_IN_PROGRESS)
  }   // if (action == SERVICE_SERIAL_BYTE
}
#endif //FEATURE_WINKEY_EMULATION