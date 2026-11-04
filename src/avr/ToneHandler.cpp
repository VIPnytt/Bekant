#ifdef ARDUINO_ARCH_AVR

#include "avr/ToneHandler.h"

#include "avr/TimeHandler.h"
#include "avr/constants.h"

#include <avr/interrupt.h>
#include <avr/io.h>
#include <wiring.h>

/**
 * @brief Timer2 Compare Match A ISR toggles PA7 directly in hardware.
 *
 * Writing a 1 to PINA toggles the corresponding PORTA bit in a single cycle.
 */
ISR(TIMER2_COMPA_vect) { PINA = 0b1U << PA7; }

/**
 * @brief Configures PA7 as a standard output and ensures Timer2 starts stopped.
 */
void ToneHandler::begin()
{
    pinMode(Pin::tone, OUTPUT);
    digitalWrite(Pin::tone, LOW);
    TCCR2A = 0U;
    TCCR2B = 0U;
    TIMSK2 = 0U;
    TCNT2 = 0U;
    active = false;
}

/**
 * @brief Asynchronously silences the tone once durationMillis has elapsed.
 */
void ToneHandler::handle()
{
    if (active && (TimeHandler::millis() - startMillis >= durationMillis))
    {
        stop();
    }
}

/**
 * @brief Starts non-blocking square-wave playback on PA7 using Timer2 CTC interrupts.
 *
 * Prescaler 8 at 8 MHz gives 1 MHz (1 tick = 1 us).
 * Half-period for frequency f is 500,000 / f ticks.
 *
 * @param frequency Audio frequency in hertz (8 Hz to 65,535 Hz).
 * @param duration Duration in milliseconds.
 */
void ToneHandler::play(unsigned int frequency, unsigned int duration)
{
    stop();
    if (frequency < 8U)
    {
        return;
    }
    OCR2A = static_cast<uint16_t>((500'000UL / frequency) - 1UL);
    TCNT2 = 0U;
    TCCR2A = 0U;
    TIFR2 = 0b1U << OCF2A;
    TIMSK2 = 0b1U << OCIE2A;
    TCCR2B = (0b1U << WGM22) | (0b1U << CS21);
    startMillis = TimeHandler::millis();
    durationMillis = duration;
    active = true;
}

/**
 * @brief Silences the tone and disables the Timer2 clock and interrupt.
 */
void ToneHandler::stop()
{
    TIMSK2 = 0U;
    TCCR2B = 0U;
    TCCR2A = 0U;
    TIFR2 = 0b1U << OCF2A;
    digitalWrite(Pin::tone, LOW);
    active = false;
}

#endif // ARDUINO_ARCH_AVR
