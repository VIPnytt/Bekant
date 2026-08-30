#ifdef ARDUINO_ARCH_AVR

#include "avr/ButtonHandler.h"

#include "avr/DeskService.h"
#include "avr/constants.h"

#include <HardwareSerial.h>

/**
 * @brief Processes button state changes and initiates movement targets.
 *
 * Reports button transitions over the serial interface and updates the button
 * sequence count when buttons are pressed.
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
        Serial1.printf("d%u\n", static_cast<unsigned char>(stateDown));
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
        Serial1.printf("u%u\n", static_cast<unsigned char>(stateUp));
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
