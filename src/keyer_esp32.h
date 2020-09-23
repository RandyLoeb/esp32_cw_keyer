#ifndef KEYER_ESP32_H
#define KEYER_ESP32_H
#include <Arduino.h>

#define GENERIC_CHARGRAB

//#define M5CORE

#if !defined M5CORE
#define REMOTE_KEYER
#endif

#ifndef M5CORE
#define TONE_ON
#define TRANSMIT

#endif

//#define ESPNOW_ONLY
//#define ESPNOW

//#define REMOTE_UDP

#define KEYER_WEBSERVER
#define TEST_BEACON
//#define REMOTE_DITDAHMODE
#if !defined REMOTE_DITDAHMODE
#define REMOTE_CHARMODE
#endif

#if defined TRANSMIT
#define TRANSMIT_PIN GPIO_NUM_23 //GPIO_NUM_15 //GPIO_NUM_4 //GPIO_NUM_2 //GPIO_NUM_15
#endif

#endif