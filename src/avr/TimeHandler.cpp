#ifdef ARDUINO_ARCH_AVR

#include "avr/TimeHandler.h"

#include "avr/ConsoleHandler.h"
#include "avr/ControllerService.h"

#include <avr/interrupt.h>
#include <avr/io.h>

namespace Timers
{
volatile unsigned long timer0{0UL};
} // namespace Timers

/**
 * @brief Timer0 Compare Match A ISR executing once every 1.0 ms.
 */
ISR(TIMER0_COMPA_vect) { ++Timers::timer0; }

void TimeHandler::begin()
{
    TCCR0A = _BV(WGM01);
    TCCR0B = _BV(CS01) | _BV(CS00);
    OCR0A = static_cast<uint8_t>((static_cast<unsigned long>(F_CPU) / ((0b1UL << 6U) * 1000UL)) - 1UL);
    TCNT0 = 0U;
    TIFR0 |= _BV(OCF0A);
    TIMSK0 |= _BV(OCIE0A);
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
