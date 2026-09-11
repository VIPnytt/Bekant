#ifdef __AVR__

#include "avr/ButtonHandler.h"

#include "avr/ConsoleHandler.h"
#include "avr/ControllerService.h"
#include "avr/ToneHandler.h"

#ifdef __AVR_ATtiny841__
#include <wiring.h>
#endif // __AVR_ATtiny841__

void ButtonHandler::begin()
{
    pinMode(pinDown, INPUT_PULLUP);
    pinMode(pinUp, INPUT_PULLUP);
}

/**
 * @brief Handles button state changes and processes the resulting input.
 *
 * Updates the press sequence and timing state, cancels movement when a button
 * is released, reports state transitions over the serial interface, and
 * processes the resulting button input.
 */
void ButtonHandler::handle()
{
    const bool _buttonDown{digitalRead(pinDown) == LOW};
    const bool _buttonUp{digitalRead(pinUp) == LOW};
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
            stop();
        }
        console.send(ConsoleHandler::State::BUTTON_DOWN, static_cast<unsigned char>(stateDown));
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
            stop();
        }
        console.send(ConsoleHandler::State::BUTTON_UP, static_cast<unsigned char>(stateUp));
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
        controller.recalibrate();
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
    else if (count == 1 && !stateUp && millis() - lastMillis > 0b1U << 8U)
    {
        count = 0;
        controller.setTarget(controller.getPresetHigh());
    }
    else if (count == -1 && !stateDown && millis() - lastMillis > 0b1U << 8U)
    {
        count = 0;
        controller.setTarget(controller.getPresetLow());
    }
    else if (count == 2 && !stateUp && millis() - lastMillis > 0b1U << 8U)
    {
        count = 0;
        controller.setPresetHigh(controller.getEncoderMin());
        ToneHandler::play(0b1U << 12U);
    }
    else if (count == -2 && !stateDown && millis() - lastMillis > 0b1U << 8U)
    {
        count = 0;
        controller.setPresetLow(controller.getEncoderMax());
        ToneHandler::play(0b1U << 12U);
    }
    else if (count != 0 && !stateDown && !stateUp && millis() - lastMillis > 0b1U << 8U)
    {
        count = 0;
    }
}

/**
 * @brief Sets a bounded target for lowering the desk.
 */
void ButtonHandler::incrementDown()
{
    const unsigned int maxCurrent{controller.getEncoderMax()};
    controller.setTarget(maxCurrent > LegHandler::minLimit + LegHandler::maxDelta ? maxCurrent - LegHandler::maxDelta
                                                                                  : LegHandler::minLimit);
}

/**
 * @brief Sets the desk's upward movement target and starts movement.
 */
void ButtonHandler::incrementUp()
{
    const unsigned int minCurrent{controller.getEncoderMin()};
    controller.setTarget(minCurrent < LegHandler::maxLimit - LegHandler::maxDelta ? minCurrent + LegHandler::maxDelta
                                                                                  : LegHandler::maxLimit);
}

/**
 * @brief Finalizes manual movement when a button is released.
 *
 * While the controller is moving, sets a final bounded target in the active
 * direction and clears the accumulated button sequence.
 */
void ButtonHandler::stop()
{
    const ControllerService::State state{controller.getState()};
    if (state == ControllerService::State::DOWN)
    {
        incrementDown();
        count = 0;
    }
    else if (state == ControllerService::State::UP)
    {
        incrementUp();
        count = 0;
    }
}

#endif // __AVR__
