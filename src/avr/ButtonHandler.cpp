#ifdef ARDUINO_ARCH_AVR

#include "avr/ButtonHandler.h"

#include "avr/DeskService.h"
#include "avr/constants.h"

#include <HardwareSerial.h>
#include <wiring.h>

/**
 * @brief Handles button state transitions and processes the resulting input.
 *
 * Updates the press sequence count, records press timing, cancels movement on
 * release, and reports transitions over the serial interface.
 */
void ButtonHandler::handle()
{
    const bool _buttonDown{digitalRead(Pin::buttonDown) == LOW};
    const bool _buttonUp{digitalRead(Pin::buttonUp) == LOW};
    if (_buttonDown != stateDown)
    {
        stateDown = _buttonDown;
        if (stateDown)
        {
            lastMillis = millis();
            --count;
        }
        else
        {
            cancel();
        }
        Serial1.write(static_cast<int>('d'));
        Serial1.write(static_cast<int>(stateDown ? '1' : '0'));
        Serial1.write(static_cast<int>('\n'));
    }
    if (_buttonUp != stateUp)
    {
        stateUp = _buttonUp;
        if (stateUp)
        {
            lastMillis = millis();
            ++count;
        }
        else
        {
            cancel();
        }
        Serial1.write(static_cast<int>('u'));
        Serial1.write(static_cast<int>(stateUp ? '1' : '0'));
        Serial1.write(static_cast<int>('\n'));
    }
    process();
}

/**
 * @brief Processes button input for desk movement, recalibration, and preset operations.
 *
 * Handles button combinations and press sequences to start movement, initiate
 * recalibration, store or recall low and high position presets, and reset
 * incomplete sequences after a timeout.
 */
void ButtonHandler::process()
{
    if (stateDown && stateUp && millis() - lastMillis > 0b1U << 13U)
    {
        count = 0;
        desk.recalibrate();
    }
    else if (stateDown && !stateUp && millis() - lastMillis > 0b1U << 9U)
    {
        count = 0;
        incrementDown();
    }
    else if (stateUp && !stateDown && millis() - lastMillis > 0b1U << 9U)
    {
        count = 0;
        incrementUp();
    }
    else if (count == -2 && millis() - lastMillis > 0b1U << 8U)
    {
        count = 0;
        desk.tone(0b1U << 12U);
        desk.setPresetLow(desk.getEncoderMax());
    }
    else if (count == -1 && millis() - lastMillis > 0b1U << 8U)
    {
        count = 0;
        desk.setTarget(desk.getPresetLow());
    }
    else if (count == 1 && millis() - lastMillis > 0b1U << 8U)
    {
        count = 0;
        desk.setTarget(desk.getPresetHigh());
    }
    else if (count == 2 && millis() - lastMillis > 0b1U << 8U)
    {
        count = 0;
        desk.tone(0b1U << 12U);
        desk.setPresetHigh(desk.getEncoderMin());
    }
    else if (count != 0 && millis() - lastMillis > 0b1U << 8U)
    {
        count = 0;
    }
}

/**
 * @brief Sets a bounded target for lowering the desk.
 */
void ButtonHandler::incrementDown()
{
    const unsigned int maxCurrent{desk.getEncoderMax()};
    desk.setTarget(maxCurrent > Encoder::minLimit + Encoder::maxDelta ? maxCurrent - Encoder::maxDelta
                                                                      : Encoder::minLimit);
}

/**
 * @brief Sets the desk's upward movement target and starts movement.
 */
void ButtonHandler::incrementUp()
{
    const unsigned int minCurrent{desk.getEncoderMin()};
    desk.setTarget(minCurrent < Encoder::maxLimit - Encoder::maxDelta ? minCurrent + Encoder::maxDelta
                                                                      : Encoder::maxLimit);
}

/**
 * @brief Resumes movement in the direction indicated by the desk state.
 */
void ButtonHandler::cancel()
{
    const DeskService::State state{desk.getState()};
    if (state == DeskService::State::DOWN)
    {
        incrementDown();
    }
    else if (state == DeskService::State::UP)
    {
        incrementUp();
    }
}

#endif // ARDUINO_ARCH_AVR
