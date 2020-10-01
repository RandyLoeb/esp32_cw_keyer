#include "keyer_esp32.h"
//#define M5CORE

#include <stdio.h>
#include <queue>
#include <string>
#include "keyer_hardware.h"
#include "keyer_features_and_options.h"
#include "keyer.h"
#include "keyer_pin_settings.h"
#include "keyer_settings.h"
#include "display.h"
#include "config.h"
#if !defined M5CORE
#include "potentiometer.h"
#endif
#include "cw_utils.h"

#if defined(ESP32)
void add_to_send_buffer(byte incoming_serial_byte);

#if defined M5CORE
#include <M5Stack.h>
#endif

#include "keyer_esp32now.h"

#endif

#include "tone/keyerTone.h"

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

#include "webServer/wifiUtils.h"
WifiUtils wifiUtils{};

#include "webServer/webServer.h"

std::queue<String> injectedText;

#include "timerStuff/timerStuff.h"
#include "virtualPins/keyerPins.h"

#if defined REMOTE_KEYER && defined REMOTE_UDP
#include "AsyncUDP.h"
AsyncUDP udp;
#endif

//todo move
void send_char(byte cw_char, byte omit_letterspace, bool display);

void setup()
{

#if defined M5CORE
  M5.begin();

#endif
  Serial.begin(115200);
  Serial.println("In setup()");

#if !defined ESPNOW_ONLY && defined KEYER_WEBSERVER
  //the web server needs a wifiutils object to handle wifi config
  //also needs place to inject text
  keyerWebServer = new KeyerWebServer(&wifiUtils, &injectedText, &configControl);

#endif

  initialize_virtualPins();

  initialize_keyer_state();
  configControl.initialize(primary_serial_port);

#if !defined M5CORE && !defined REMOTE_KEYER
  // potentiometer
  wpmPot.initialize(potentiometer_pin);
  configControl.configuration.pot_activated = 1;
#endif

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
  Serial.println("Calling juststation");
  wifiUtils.justStation();
#endif

  Serial.print("My MAC is:");
  Serial.println(wifiUtils.getMac());

#if defined ESPNOW
  Serial.println("Calling espnow initialization");
  initialize_espnow_wireless();
  _ditdahCallBack = &ditdahCallBack;
#endif
  Serial.println("Calling initializing timer");
  initializeTimerStuff(&configControl);
  detectInterrupts = true;

  Serial.println("Calling initializing display control");
  displayControl.initialize([](char x) {
    send_char(x, KEYER_NORMAL, false);
  });

#if defined REMOTE_KEYER
#if defined REMOTE_UDP
  if (udp.listen(1234))
  {
    udp.onPacket([](AsyncUDPPacket packet) {
      /* Serial.print("Received data: ");
      Serial.write(packet.data(), packet.length());
      Serial.println(); */
      String myString = (const char *)packet.data();
      //Serial.println(myString);
      PaddlePressDetection *newPd;
      newPd = new PaddlePressDetection();
      newPd->Detected = myString == "dit" ? DitOrDah::DIT : DitOrDah::DAH;
      newPd->Display = true;
      newPd->Source = PaddlePressSource::ARTIFICIAL;
      ditsNdahQueue.push(newPd);
    });
  }
#endif
#endif
}

// --------------------------------------------------------------------------------------------

void loop()
{

#if !defined M5CORE && !defined REMOTE_KEYER
  wpmPot.checkPotentiometer(wpmSetCallBack);
#endif

  if (configControl.config_dirty)
  {
    Serial.println("dirty config!");
    configControl.save();

    configControl.config_dirty = 0;
  }

  displayControl.service_display();

  service_millis_rollover();
  //wifiUtils.processNextDNSRequest();

#if !defined ESPNOW_ONLY && defined KEYER_WEBSERVER
  keyerWebServer->handleClient();
#endif
  service_injected_text();

  processDitDahQueue();

#if defined TEST_BEACON && !defined REMOTE_KEYER
  if (ditsNdahQueue.empty())
  {
    delay(5000);
    String testText = "THE QUICK BROWN FOX JUMPED OVER THE LAZY DOGS. 1234567890";
    injectedText.push(testText);
  }
#endif
}

void send_char(byte cw_char, byte omit_letterspace, bool display = true)
{
  PaddlePressDetection *newPd;
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
      newPd = new PaddlePressDetection();
      newPd->Detected = DitOrDah::SPACE;
      newPd->Display = display;
      newPd->Source = PaddlePressSource::ARTIFICIAL;
      ditsNdahQueue.push(newPd);
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
        for (int i = 0; i < ditsanddahs.length(); i++)
        {
          newPd = new PaddlePressDetection();
          newPd->Detected = ditsanddahs.charAt(i) == '.' ? DitOrDah::DIT : DitOrDah::DAH;
          newPd->Source = PaddlePressSource::ARTIFICIAL;
          newPd->Display = display;
          ditsNdahQueue.push(newPd);
        }
        newPd = new PaddlePressDetection();
        newPd->Detected = DitOrDah::FORCED_CHARSPACE;
        newPd->Source = PaddlePressSource::ARTIFICIAL;
        newPd->Display = display;
        ditsNdahQueue.push(newPd);
        //send_the_dits_and_dahs(ditsanddahs.c_str());
      }
    }
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
      send_char(x, KEYER_NORMAL);
    }
    injectedText.pop();
  }
}

void initialize_keyer_state()
{

  key_state = 0;
  key_tx = 1;

#ifndef FEATURE_SO2R_BASE
  //switch_to_tx_silent(1);
#endif
}

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
