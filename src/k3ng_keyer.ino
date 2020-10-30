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
  Serial.println("In setup()");

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
  initializeTimerStuff(&configControl, cwControl);
  detectInterrupts = true;

  Serial.println("Calling initializing display control");
  displayControl.initialize([](char x) {
    cwControl->send_char(x, KEYER_NORMAL, false);
  });
}

// --------------------------------------------------------------------------------------------

void loop()
{

  if (configControl.config_dirty)
  {
    Serial.println("dirty config!");
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
