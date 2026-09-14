#ifdef ARDUINO_ARCH_AVR

#include "avr/ToneHandler.h"

#include "avr/constants.h"

#include <wiring.h>

/**
 * @brief Configures the tone pin as an output.
 */
void ToneHandler::begin() { pinMode(Pin::tone, OUTPUT); }

/**
 * @brief Generates a blocking square-wave tone on the tone output.
 *
 * Unsupported frequencies and zero-duration requests produce no output.
 *
 * @param frequency Tone frequency in hertz.
 * @param duration Approximate playback duration in milliseconds.
 */
void ToneHandler::play(unsigned int frequency, unsigned long duration)
{
    if (duration == 0UL || frequency < minFrequency || frequency > maxFrequency)
    {
        return;
    }
    const unsigned int halfPeriod{static_cast<unsigned int>(500'000UL / frequency)};
    if (halfPeriod <= overhead)
    {
        return;
    }
    const unsigned int delay{halfPeriod - overhead};
    for (unsigned long idx{0UL}; idx < (duration * 1000UL) / (2UL * halfPeriod); ++idx)
    {
        digitalWrite(Pin::tone, HIGH);
        delayMicroseconds(delay);
        digitalWrite(Pin::tone, LOW);
        delayMicroseconds(delay);
    }
}

#endif // ARDUINO_ARCH_AVR
