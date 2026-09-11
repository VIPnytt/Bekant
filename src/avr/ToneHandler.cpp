#ifdef __AVR__

#include "avr/ToneHandler.h"

#ifdef __AVR_ATtiny841__
#include <wiring.h>
#elif defined(__AVR_ATtiny1624__)
#include <api/Common.h>
#endif // __AVR_ATtiny841__

void ToneHandler::begin() { pinMode(pin, OUTPUT); }

/**
 * @brief Generates a square-wave tone at the specified frequency.
 *
 * @param frequency Tone frequency in hertz.
 */
void ToneHandler::play(unsigned int frequency)
{
    if (frequency != 0U)
    {
        const unsigned int halfPeriod{static_cast<unsigned int>(500'000UL / frequency)};
        const unsigned int delay{static_cast<unsigned int>(halfPeriod - (48'000'000UL / F_CPU))};
        for (unsigned long idx{0UL}; idx < (0b1UL << 17U) / halfPeriod; ++idx)
        {
            digitalWrite(pin, HIGH);
            delayMicroseconds(delay);
            digitalWrite(pin, LOW);
            delayMicroseconds(delay);
        }
    }
}

#endif // __AVR__
