#ifndef KEYER_ESP32_H
#define KEYER_ESP32_H
#include <Arduino.h>

#define GENERIC_CHARGRAB

#define M5CORE

#if !defined M5CORE
#define REMOTE_KEYER
#endif

#define TONE_ON
#ifndef M5CORE

#define TRANSMIT

#endif

//#define ESPNOW_ONLY
//#define ESPNOW

//#define REMOTE_UDP

#define KEYER_WEBSERVER
//#define TEST_BEACON
//#define REMOTE_DITDAHMODE
#if !defined REMOTE_DITDAHMODE
#define REMOTE_CHARMODE
#endif

// transmitter pin
#if defined TRANSMIT
#define TRANSMIT_PIN GPIO_NUM_23 //GPIO_NUM_15 //GPIO_NUM_4 //GPIO_NUM_2 //GPIO_NUM_15
#endif

//displays, probably you need to make sure only 1 is defined
#if defined M5CORE
#define M5DISPLAY
#else
#define OLED64X128
#endif

//tone pin
#if defined M5CORE
// pins known to work on m5 = 26,25,17,16
// not work on m5 = 35, 36,3,1 (works but screws up serial printlns?),
#define TONE_PIN GPIO_NUM_25
#else
#define TONE_PIN GPIO_NUM_22
#endif

//ditdah pins
#if !defined REMOTE_KEYER
#define HAS_DITDAHPINS 1
#endif

#if defined HAS_DITDAHPINS
#if defined M5CORE
#define DITPIN1 GPIO_NUM_16 //GPIO_NUM_2
#define DITPIN2 GPIO_NUM_39
#define DAHPIN1 GPIO_NUM_17 //GPIO_NUM_5
#define DAHPIN2 GPIO_NUM_37
#endif

#endif

#endif