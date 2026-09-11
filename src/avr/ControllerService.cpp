#ifdef __AVR__

#include "avr/ControllerService.h"

#include "avr/ConsoleHandler.h"
#include "avr/ToneHandler.h"

#include <EEPROM.h>
#include <avr/wdt.h>

#ifdef __AVR_ATtiny841__
#include <wiring.h>
#endif // __AVR_ATtiny841__

/**
 * @brief Initializes communication, hardware pins, presets, the watchdog, and the LIN interface.
 *
 * Sends the firmware fingerprint and stored presets to the ESP32.
 * Reports a LIN initialization failure and sounds a tone when initialization does not succeed.
 */
void ControllerService::begin()
{
    console.begin();
    delay(0b1UL << 10U);
#ifdef __AVR_ATtiny841__
    wdt_enable(WDTO_8S);
#elif defined(__AVR_ATtiny1624__)
    wdt_enable(WDTO_2S);
#endif // __AVR_ATtiny841__
    button.begin();
    ToneHandler::begin();
    EEPROM.get<unsigned int>(static_cast<int>('h'), presetHigh);
    EEPROM.get<unsigned int>(static_cast<int>('l'), presetLow);
    console.send(ConsoleHandler::State::VERSION, fingerprint(version));
    console.send(ConsoleHandler::State::PRESET_HIGH, presetHigh);
    console.send(ConsoleHandler::State::PRESET_LOW, presetLow);
    const unsigned char init{leg.begin()};
    if (init != 0U)
    {
        console.send(ConsoleHandler::State::INITIALIZATION, init);
        ToneHandler::play(0b1U << 8U);
        return;
    }
    wdt_reset();
}

/**
 * @brief Reads encoder data, advances the controller state, and handles user input outside recalibration states.
 */
void ControllerService::handle()
{
    if (read())
    {
        process();
    }
    if (state != State::RECAL_PREPARE && state != State::RECAL_ONGOING && state != State::RECAL_DONE)
    {
        console.handle();
        button.handle();
    }
}

/**
 * @brief Polls both desk encoders and updates their positions and states.
 *
 * Reports encoder communication failures and sounds an alert when movement is
 * pending and either request fails. Resets the watchdog after both requests
 * succeed. Node data and communication errors are forwarded only when they
 * change, including when a node recovers from an error.
 *
 * @return true if both encoder requests succeed, false otherwise.
 */
bool ControllerService::read()
{
    constexpr unsigned char empty[3U]{0U, 0U, 0U};
    leg.sendResponse(LegHandler::getPid(0x11U), empty);
    unsigned char node[3U]{};
    const unsigned char _error8{leg.getLeg(LegHandler::getPid(0x8U), node)};
    if (_error8 == 0U)
    {
        const unsigned int _encoder8{static_cast<unsigned int>(node[0U]) | static_cast<unsigned int>(node[1U]) << 8U};
        if (_encoder8 != encoder8 || node[2U] != state8 || _error8 != error8)
        {
            if (_encoder8 != encoder8)
            {
                lastMillis = millis();
            }
            encoder8 = _encoder8;
            state8 = node[2U];
            error8 = 0U;
            console.send(ConsoleHandler::State::NODE8, node);
        }
    }
    else if (_error8 != error8)
    {
        error8 = _error8;
        console.send(ConsoleHandler::State::NODE8, error8);
    }
    const unsigned char _error9{leg.getLeg(LegHandler::getPid(0x9U), node)};
    if (_error9 == 0U)
    {
        const unsigned int _encoder9{static_cast<unsigned int>(node[0U]) | static_cast<unsigned int>(node[1U]) << 8U};
        if (_encoder9 != encoder9 || node[2U] != state9 || _error9 != error9)
        {
            if (_encoder9 != encoder9)
            {
                lastMillis = millis();
            }
            encoder9 = _encoder9;
            state9 = node[2U];
            error9 = 0U;
            console.send(ConsoleHandler::State::NODE9, node);
        }
    }
    else if (_error9 != error9)
    {
        error9 = _error9;
        console.send(ConsoleHandler::State::NODE9, error9);
    }
    if (_error8 != 0U || _error9 != 0U)
    {
        if (pending)
        {
            ToneHandler::play(0b1U << 8U);
        }
        return false;
    }
    wdt_reset();
    return true;
}

/**
 * @brief Advances the desk movement and recalibration state machine.
 *
 * Processes the current state, issues required movement or calibration commands,
 * and transitions to the next state.
 */
void ControllerService::process()
{
    switch (state)
    {
    case State::IDLE:
        handleStateIdle();
        break;
    case State::PREPARE:
        handleStatePrepare();
        break;
    case State::DOWN:
        handleStateDown();
        break;
    case State::UP:
        handleStateUp();
        break;
    case State::STOP:
        state = State::DONE;
        sendCommand(LegHandler::Command::OK);
        break;
    case State::DONE:
        handleStateDone();
        break;
    case State::RECAL_PREPARE:
        state = State::RECAL_ONGOING;
        sendCommand(LegHandler::Command::PRE_MOVE);
        break;
    case State::RECAL_ONGOING:
        handleStateRecalOngoing();
        break;
    case State::RECAL_DONE:
        state = State::IDLE;
        leg.sendCommand(LegHandler::Command::CALIBRATE_END, 99U);
        break;
    }
}

/**
 * @brief Determines whether both desk nodes report an idle-compatible status.
 *
 * @return `true` if both nodes report an idle-compatible status, `false` otherwise.
 */
bool ControllerService::isIdle() const
{
    return (state8 == 0U || state8 == 0x25U || state8 == 0x60U) && (state9 == 0U || state9 == 0x25U || state9 == 0x60U);
}

/**
 * @brief Starts movement preparation when a target is pending and both nodes are idle; otherwise keeps the nodes idle.
 */
void ControllerService::handleStateIdle()
{
    if (pending && isIdle())
    {
        state = State::PREPARE;
        return;
    }
    sendCommand(LegHandler::Command::IDLE);
}

/**
 * @brief Prepares movement toward the requested target.
 *
 * Adjusts the target by the configured offset when necessary, sends the pre-movement
 * command, or clears the pending request when the target is already within range.
 */
void ControllerService::handleStatePrepare()
{
    if (encoderTarget < getEncoderMin())
    {
        if (encoderTarget >= LegHandler::minLimit + LegHandler::targetOffset)
        {
            encoderTarget -= LegHandler::targetOffset;
        }
        state = State::DOWN;
    }
    else if (encoderTarget > getEncoderMax())
    {
        if (encoderTarget <= LegHandler::maxLimit - LegHandler::targetOffset)
        {
            encoderTarget += LegHandler::targetOffset;
        }
        state = State::UP;
    }
    else
    {
        pending = false;
        state = State::IDLE;
        return;
    }
    lastMillis = millis();
    sendCommand(LegHandler::Command::PRE_MOVE);
}

/**
 * @brief Continues lowering the desk until the target is reached or movement times out.
 *
 * Transitions to the stop state when either encoder reaches the target or the encoder response timeout expires.
 */
void ControllerService::handleStateDown()
{
    if (encoderTarget >= getEncoderMin() || millis() - lastMillis > (0b1U << 8U))
    {
        state = State::STOP;
        return;
    }
    sendCommand(LegHandler::Command::LOWER);
}

/**
 * @brief Continues raising the desk until the target is reached or encoder updates time out.
 */
void ControllerService::handleStateUp()
{
    if (encoderTarget <= getEncoderMax() || millis() - lastMillis > (0b1U << 8U))
    {
        state = State::STOP;
        return;
    }
    sendCommand(LegHandler::Command::RAISE);
}

/**
 * @brief Completes a movement operation once both desk nodes are idle.
 */
void ControllerService::handleStateDone()
{
    if (isIdle())
    {
        pending = false;
        state = State::IDLE;
        return;
    }
    sendCommand(LegHandler::Command::FINISH);
}

/**
 * @brief Advances the ongoing recalibration process.
 *
 * Transitions to the recalibration-complete state when both nodes are ready
 * and the maximum encoder reading is within the calibration limit; otherwise,
 * continues recalibration.
 */
void ControllerService::handleStateRecalOngoing()
{
    if (state8 == 1U && state9 == 1U && getEncoderMax() <= 99U)
    {
        state = State::RECAL_DONE;
        return;
    }
    leg.sendCommand(LegHandler::Command::CALIBRATE_BEGIN, 0U);
}

/**
 * @brief Sends a movement command using a target constrained by encoder positions and safety limits.
 *
 * @param command Movement command to send.
 */
void ControllerService::sendCommand(LegHandler::Command command)
{
    const unsigned int maxCurrent{controller.getEncoderMax()};
    const unsigned int minCurrent{controller.getEncoderMin()};
    const unsigned int maxTarget{minCurrent < LegHandler::maxLimit - LegHandler::maxDelta
                                     ? minCurrent + LegHandler::maxDelta
                                     : LegHandler::maxLimit};
    const unsigned int minTarget{maxCurrent > LegHandler::minLimit + LegHandler::maxDelta
                                     ? maxCurrent - LegHandler::maxDelta
                                     : LegHandler::minLimit};
    leg.sendCommand(command, constrain(encoderTarget, minTarget, maxTarget));
}

/**
 * @brief Starts desk recalibration when both desk nodes are idle.
 */
void ControllerService::recalibrate()
{
    if (isIdle())
    {
        ToneHandler::play(0b1U << 12U);
        pending = false;
        state = State::RECAL_PREPARE;
    }
}

/**
 * @brief Updates and persists the upper desk-height preset when it is within the encoder limits.
 *
 * @param preset Upper desk-height preset to store.
 */
void ControllerService::setPresetHigh(unsigned int preset)
{
    if (preset != presetHigh && preset <= LegHandler::maxLimit && preset >= LegHandler::minLimit)
    {
        presetHigh = preset;
        EEPROM.put(static_cast<int>('h'), presetHigh);
    }
    console.send(ConsoleHandler::State::PRESET_HIGH, presetHigh);
}

/**
 * @brief Stores and reports the lower desk preset.
 *
 * The preset is stored when it differs from the current value and falls within
 * the encoder limits. The resulting preset value is reported.
 *
 * @param preset Lower desk position to store.
 */
void ControllerService::setPresetLow(unsigned int preset)
{
    if (preset != presetLow && preset <= LegHandler::maxLimit && preset >= LegHandler::minLimit)
    {
        presetLow = preset;
        EEPROM.put(static_cast<int>('l'), presetLow);
    }
    console.send(ConsoleHandler::State::PRESET_LOW, presetLow);
}

/**
 * @brief Sets the target position and marks movement as pending when accepted.
 *
 * @param position Target position; zero and `0xFFFF` leave the current target unchanged.
 */
void ControllerService::setTarget(unsigned int position)
{
    if (position != 0U && position != 0xFFFFU)
    {
        encoderTarget = position;
        pending = true;
    }
    console.send(ConsoleHandler::State::POSITION, encoderTarget);
}

/**
 * @brief Gets the greater of the two encoder values.
 *
 * @return unsigned int The greater encoder value.
 */
unsigned int ControllerService::getEncoderMax() const { return encoder8 > encoder9 ? encoder8 : encoder9; }

/**
 * @brief Gets the smaller current encoder value.
 *
 * @return unsigned int The lower value reported by the encoder nodes.
 */
unsigned int ControllerService::getEncoderMin() const { return encoder8 < encoder9 ? encoder8 : encoder9; }

/**
 * @brief Retrieves the configured high preset height.
 *
 * @return unsigned int The stored high preset value.
 */
unsigned int ControllerService::getPresetHigh() const { return presetHigh; }

/**
 * @brief Retrieves the configured low preset.
 *
 * @return unsigned int The stored low preset value.
 */
unsigned int ControllerService::getPresetLow() const { return presetLow; }

/**
 * @brief Gets the current desk service state.
 *
 * @return State Current movement or recalibration state.
 */
ControllerService::State ControllerService::getState() const { return state; }

/**
 * @brief Provides access to the shared ControllerService instance.
 *
 * @return ControllerService& Reference to the singleton instance.
 */
ControllerService &ControllerService::getInstance()
{
    static ControllerService instance;
    return instance;
}

// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
ControllerService &controller{ControllerService::getInstance()};

#endif // __AVR__
