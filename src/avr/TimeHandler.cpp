#ifdef ARDUINO_ARCH_AVR

#include "avr/TimeHandler.h"

#include <avr/interrupt.h>
#include <avr/io.h>

namespace Timers
{
volatile unsigned long timer0{0UL}; // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)
} // namespace Timers

/**
 * @brief Increments the millisecond counter on a Timer0 compare match.
 */
ISR(TIM0_COMPA_vect) { ++Timers::timer0; }

/**
 * @brief Configures Timer0 to generate millisecond compare interrupts without resetting the accumulated count.
 */
void TimeHandler::begin()
{
    TCCR0A = 0b1U << WGM01;
    TCCR0B = (0b1U << CS01) | (0b1U << CS00);
    OCR0A = static_cast<uint8_t>((static_cast<unsigned long>(F_CPU) / ((0b1UL << 6U) * 1000UL)) - 1UL);
    TCNT0 = 0U;
    TIFR0 = 0b1U << OCF0A;
    TIMSK0 |= 0b1U << OCIE0A;
}

/**
 * @brief Reads the millisecond counter atomically by protecting against torn multi-byte reads.
 */
unsigned long TimeHandler::millis()
{
    unsigned long currentMillis{};
    const uint8_t sreg{SREG};
    cli();
    currentMillis = Timers::timer0;
    SREG = sreg;
    return currentMillis;
}

#endif // ARDUINO_ARCH_AVR
