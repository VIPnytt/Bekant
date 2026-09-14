#ifdef ARDUINO_ARCH_AVR

#include "avr/ToneHandler.h"

#include "avr/constants.h"

#include <wiring.h>

void ToneHandler::begin() { pinMode(Pin::tone, OUTPUT); }

void ToneHandler::play(unsigned int frequency, unsigned long duration)
{
    if (frequency == 0U || duration == 0UL)
    {
        return;
    }
    const unsigned int halfPeriod{static_cast<unsigned int>(500'000UL / frequency)};
    const unsigned int delay{static_cast<unsigned int>(500'000UL / frequency) - overhead};
    for (unsigned long idx{0UL}; idx < (duration * 1000UL) / (2UL * halfPeriod); ++idx)
    {
        digitalWrite(Pin::tone, HIGH);
        delayMicroseconds(delay);
        digitalWrite(Pin::tone, LOW);
        delayMicroseconds(delay);
    }
}

#endif // ARDUINO_ARCH_AVR
