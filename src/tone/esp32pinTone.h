#ifndef _ESP32PINTONE_H_
#define _ESP32PINTONE_H_

#include "Arduino.h"
//#include "Config.h"
#include "toneBase.h"

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */
#include "esp32-hal-dac.h"
#ifdef __cplusplus
}
#endif /* __cplusplus */

class ESP32PINTONE : public toneBase
{
public:
    ESP32PINTONE(int pnum);
    int pin;
    int tone_pin_channel;
    void begin();
    void end();
    void mute();
    void tone(uint16_t frequency);
    void tone(uint16_t frequency, uint32_t duration);
    void beep();
    void setBeep(uint16_t frequency, uint16_t duration);
    void update();

    void write(uint8_t value);
    void setVolume(uint8_t volume);
    void playMusic(const uint8_t *music_data, uint16_t sample_rate);
    bool enabled;
    void initialize()
    {
        Serial.println("got pin speaker begin");
        this->begin();
    }
    void noTone()
    {
        this->mute();
    }

private:
    uint32_t _count;
    uint8_t _volume;
    uint16_t _beep_duration;
    uint16_t _beep_freq;
    bool _begun;
    bool speaker_on;
};
#endif