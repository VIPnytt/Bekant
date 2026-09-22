#ifdef ARDUINO_ARCH_AVR

#include "avr/ButtonHandler.h"

#include "avr/ConsoleHandler.h"
#include "avr/ControllerService.h"
#include "avr/ToneHandler.h"
#include "avr/constants.h"

#include <wiring.h>

/**
 * @brief Configures the desk buttons as pull-up inputs and the ESP32 simulation signal as an input.
 */
void ButtonHandler::begin()
{
    pinMode(Pin::button3, INPUT_PULLUP);
    pinMode(Pin::button4, INPUT_PULLUP);
    pinMode(Pin::buttonDown, INPUT_PULLUP);
    pinMode(Pin::buttonUp, INPUT_PULLUP);
    pinMode(Pin::mosi, INPUT);
}

/**
 * @brief Handles button state changes and processes the resulting input.
 *
 * Updates the press sequence and timing state, cancels movement when a button
 * is released, reports all four button states and the ESP32 simulation signal
 * over the serial interface when any button state changes, and processes the
 * resulting button input.
 */
void ButtonHandler::handle()
{
    const bool _button3{digitalRead(Pin::button3) == LOW};
    const bool _button4{digitalRead(Pin::button4) == LOW};
    const bool _buttonDown{digitalRead(Pin::buttonDown) == LOW};
    const bool _buttonUp{digitalRead(Pin::buttonUp) == LOW};
    bool pending{false};
    if (_button3 != state3)
    {
        state3 = _button3;
        pending = true;
    }
    if (_button4 != state4)
    {
        state4 = _button4;
        pending = true;
    }
    if (_buttonDown != stateDown)
    {
        stateDown = _buttonDown;
        pending = true;
        if (stateDown)
        {
            lastMillis = millis();
            --count;
        }
        else
        {
            stop();
        }
    }
    if (_buttonUp != stateUp)
    {
        stateUp = _buttonUp;
        pending = true;
        if (stateUp)
        {
            lastMillis = millis();
            ++count;
        }
        else
        {
            stop();
        }
    }
    if (pending)
    {
        ConsoleHandler::send(ConsoleHandler::State::BUTTONS,
                             static_cast<unsigned char>((stateUp ? 0b1U : 0U) | (stateDown ? 0b1U << 1U : 0U) |
                                                        (state3 ? 0b1U << 2U : 0U) | (state4 ? 0b1U << 3U : 0U) |
                                                        (digitalRead(Pin::mosi) == LOW ? 0U : 0b1U << 4U)));
    }
    process();
}

/**
 * @brief Processes button input for desk movement, recalibration, and preset operations.
 *
 * Handles button combinations and press sequences to start movement, initiate
 * recalibration, store or recall low and high position presets, and reset
 * incomplete sequences after a timeout. A confirmation tone is played after
 * storing a preset.
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
        ToneHandler::play(0b1U << 12U, 0b1U << 8U);
    }
    else if (count == -2 && !stateDown && millis() - lastMillis > 0b1U << 8U)
    {
        count = 0;
        controller.setPresetLow(controller.getEncoderMax());
        ToneHandler::play(0b1U << 12U, 0b1U << 8U);
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
    controller.setTarget(maxCurrent > Encoder::minLimit + Encoder::maxDelta ? maxCurrent - Encoder::maxDelta
                                                                            : Encoder::minLimit);
}

/**
 * @brief Sets the desk's upward movement target and starts movement.
 */
void ButtonHandler::incrementUp()
{
    const unsigned int minCurrent{controller.getEncoderMin()};
    controller.setTarget(minCurrent < Encoder::maxLimit - Encoder::maxDelta ? minCurrent + Encoder::maxDelta
                                                                            : Encoder::maxLimit);
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

#endif // ARDUINO_ARCH_AVR
