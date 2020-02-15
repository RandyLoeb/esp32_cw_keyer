void tone(uint8_t pin, unsigned short freq, unsigned duration = 0) {}
void noTone(uint8_t pin)
{
    tone(pin, -1);
}
