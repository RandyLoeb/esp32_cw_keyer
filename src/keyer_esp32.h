#ifndef KEYER_ESP32_H
#define KEYER_ESP32_H

#define GENERIC_CHARGRAB

#define M5CORE

#if !defined M5CORE
#define REMOTE_KEYER
#endif

#ifndef M5CORE
#define TONE_ON
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

#endif