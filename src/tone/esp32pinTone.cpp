#include "esp32pinTone.h"

ESP32PINTONE::ESP32PINTONE(int pnum)
{
    this->pin = pnum;
    this->tone_pin_channel = 0;
    _volume = 100;

    _begun = false;
}

void ESP32PINTONE::begin()
{
    _begun = true;
    //Serial.print("tonepin is:");
    //Serial.println(this->pin);
    ledcSetup(this->tone_pin_channel, 0, 8); //13);
    ledcAttachPin(this->pin, this->tone_pin_channel);
    setBeep(1000, 100);
}

void ESP32PINTONE::end()
{
    mute();
    ledcDetachPin(this->pin);
    _begun = false;
}

void ESP32PINTONE::tone(uint16_t frequency)
{
    //Serial.println("got tone 1 param");
    if (!_begun)
        begin();
    ledcWriteTone(this->tone_pin_channel, frequency);
    ledcWrite(this->tone_pin_channel, 0x400 >> _volume);
}

void ESP32PINTONE::tone(uint16_t frequency, uint32_t duration)
{
    //Serial.println("got tone 2 param");
    tone(frequency);
    _count = millis() + duration;
    speaker_on = 1;
}

void ESP32PINTONE::beep()
{
    if (!_begun)
        begin();
    tone(_beep_freq, _beep_duration);
}

void ESP32PINTONE::setBeep(uint16_t frequency, uint16_t duration)
{
    _beep_freq = frequency;
    _beep_duration = duration;
}

void ESP32PINTONE::setVolume(uint8_t volume)
{
    _volume = 11 - volume;
}

void ESP32PINTONE::mute()
{
    ledcWriteTone(this->tone_pin_channel, 0);
    digitalWrite(this->pin, 0);
}

void ESP32PINTONE::update()
{
    if (speaker_on)
    {
        if (millis() > _count)
        {
            speaker_on = 0;
            mute();
        }
    }
}

void ESP32PINTONE::write(uint8_t value)
{
    dacWrite(this->pin, value);
}

void ESP32PINTONE::playMusic(const uint8_t *music_data, uint16_t sample_rate)
{
    uint32_t length = strlen((char *)music_data);
    uint16_t delay_interval = ((uint32_t)1000000 / sample_rate);
    if (_volume != 11)
    {
        for (int i = 0; i < length; i++)
        {
            dacWrite(this->pin, music_data[i] / _volume);
            delayMicroseconds(delay_interval);
        }

        for (int t = music_data[length - 1] / _volume; t >= 0; t--)
        {
            dacWrite(this->pin, t);
            delay(2);
        }
    }
    // ledcSetup(this->tone_pin_channel, 0, 13);
    ledcAttachPin(this->pin, this->tone_pin_channel);
}
