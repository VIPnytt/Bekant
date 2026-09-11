#pragma once

#ifdef __AVR__

#ifdef __AVR_ATtiny841__
#include <core_pins.h>
#elif defined(__AVR_ATtiny1624__)
#include <pins_arduino.h>
#endif // __AVR_ATtiny841__

class ToneHandler
{
private:
    static constexpr unsigned char pin{PIN_PA7};

public:
    static void begin();

    static void play(unsigned int frequency);
};

#endif // __AVR__
