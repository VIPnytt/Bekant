#ifdef ARDUINO_ARCH_AVR

#include "avr/DeskService.h"

#include "avr/constants.h"

#include <EEPROM.h>
#include <HardwareSerial.h>
#include <avr/wdt.h>
#include <wiring.h>

/**
 * @brief Initializes serial communication, hardware pins, stored presets, and the LIN interface.
 *
 * Transmits the firmware version, valid stored presets, and required LIN initialization
 * packets. Reports initialization errors with a tone and a serial status message.
 */
void DeskService::begin()
{
    Serial1.begin(115'200UL);
    delay(0b1UL << 11U);
    wdt_enable(WDTO_8S);
    pinMode(Pin::buttonDown, INPUT_PULLUP);
    pinMode(Pin::buttonUp, INPUT_PULLUP);
    pinMode(Pin::tone, OUTPUT);
    EEPROM.get<unsigned int>(static_cast<int>('h'), presetHigh);
    EEPROM.get<unsigned int>(static_cast<int>('l'), presetLow);
    Serial1.print("Bekant\nv1.0.0\n");
    console.send(presetHigh <= Encoder::maxLimit && presetHigh >= Encoder::minLimit ? 'h' : 'H', presetHigh);
    console.send(presetLow <= Encoder::maxLimit && presetLow >= Encoder::minLimit ? 'l' : 'L', presetLow);
    lin.begin();
    constexpr unsigned char data[21U][4U]{
        {0xFFU, 0x7U, 0xFFU, 0xFFU},
        {0xFFU, 0x7U, 0xFFU, 0xFFU},
        {0xFFU, 0x1U, 0x7U, 0xFFU},
        {0xD0U, 0x2U, 0x7U, 0xFFU},
        {0x0U, 0x2U, 0x7U, 0xFFU},
        {0x0U, 0x6U, 0x9U, 0x0U},
        {0x0U, 0x6U, 0xCU, 0x0U},
        {0x0U, 0x6U, 0xDU, 0x0U},
        {0x0U, 0x6U, 0xAU, 0x0U},
        {0x0U, 0x6U, 0xBU, 0x0U},
        {0x0U, 0x4U, 0x0U, 0x0U},
        {0x0U, 0x2U, 0x0U, 0x0U},
        {0x0U, 0x6U, 0x9U, 0x0U},
        {0x0U, 0x6U, 0xCU, 0x0U},
        {0x0U, 0x6U, 0xDU, 0x0U},
        {0x0U, 0x6U, 0xAU, 0x0U},
        {0x0U, 0x6U, 0xBU, 0x0U},
        {0x0U, 0x4U, 0x1U, 0x0U},
        {0x0U, 0x2U, 0x1U, 0x0U},
        {0xD0U, 0x1U, 0x7U, 0x0U},
        {0xD0U, 0x2U, 0x7U, 0x0U},
    };
    unsigned char pid{0U};
    for (unsigned char idx{0U}; idx < static_cast<unsigned char>(sizeof(data) / sizeof(data[0U])); ++idx)
    {
        switch (idx)
        {
        case 4U:
        case 11U:
            for (; pid < 8U; ++pid)
            {
                if (sendPacket(pid, data[idx][1U], data[idx][2U], data[idx][3U]))
                {
                    break;
                }
            }
            if (pid == 8U)
            {
                Serial1.write(static_cast<int>('I'));
                Serial1.write(static_cast<int>('\n'));
                tone(0b1U << 8U);
                return;
            }
            break;
        case 18U:
            for (; pid < 8U; ++pid)
            {
                sendPacket(pid, data[idx][1U], data[idx][2U], data[idx][3U]);
            }
            break;
        default:
            sendPacket(data[idx][0U] == 0U ? pid : data[idx][0U], data[idx][1U], data[idx][2U], data[idx][3U]);
            break;
        }
    }
    constexpr unsigned char magicPacket[3U]{0xF6U, 0xFFU, 0xBFU};
    lin.send(0x12U, magicPacket);
    wdt_reset();
}

/**
 * @brief Sends a LIN command packet and requests its response.
 *
 * @param byte1 First command byte.
 * @param byte2 Second command byte.
 * @param byte3 Third command byte.
 * @param byte4 Fourth command byte.
 * @return `true` if the response request succeeds, `false` otherwise.
 */
bool DeskService::sendPacket(unsigned char byte1, unsigned char byte2, unsigned char byte3, unsigned char byte4)
{
    const unsigned char packet[8U]{byte1, byte2, byte3, byte4, 0xFFU, 0xFFU, 0xFFU, 0xFFU};
    lin.send(0x3C, packet);
    unsigned char response[sizeof(packet)]{};
    return lin.request(0x3DU, response);
}

/**
 * @brief Reads encoder data, advances the state machine on successful communication, and handles user input outside
 * recalibration states.
 */
void DeskService::handle()
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
 * Reports changed positions and communication failures. Cancels pending
 * movement and sounds an alert when either encoder request fails.
 *
 * @return true if both encoder requests succeed, false otherwise.
 */
bool DeskService::read()
{
    constexpr unsigned char empty[3U]{0U, 0U, 0U};
    lin.send(0x11U, empty);
    unsigned char nodeA[3U]{};
    unsigned char nodeB[3U]{};
    const bool validA{lin.request(0x8U, nodeA)};
    const bool validB{lin.request(0x9U, nodeB)};
    if (validA)
    {
        const unsigned int _encoderA{static_cast<unsigned int>(nodeA[0U]) | static_cast<unsigned int>(nodeA[1U] << 8U)};
        stateA = nodeA[2U];
        if (_encoderA != encoderA)
        {
            encoderA = _encoderA;
            lastMillis = millis();
            console.send('a', encoderA);
        }
    }
    else
    {
        Serial1.write(static_cast<int>('A'));
        Serial1.write(static_cast<int>('\n'));
        if (pending)
        {
            tone(0b1U << 8U);
        }
    }
    if (validB)
    {
        const unsigned int _encoderB{static_cast<unsigned int>(nodeB[0U]) | static_cast<unsigned int>(nodeB[1U] << 8U)};
        stateB = nodeB[2U];
        if (_encoderB != encoderB)
        {
            encoderB = _encoderB;
            lastMillis = millis();
            console.send('b', encoderB);
        }
    }
    else
    {
        Serial1.write(static_cast<int>('B'));
        Serial1.write(static_cast<int>('\n'));
        if (pending)
        {
            tone(0b1U << 8U);
        }
    }
    if (!validA || !validB)
    {
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
void DeskService::process()
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
        sendCommand(Command::OK);
        break;
    case State::DONE:
        handleStateDone();
        break;
    case State::RECAL_PREPARE:
        state = State::RECAL_ONGOING;
        sendCommand(Command::PRE_MOVE);
        break;
    case State::RECAL_ONGOING:
        handleStateRecalOngoing();
        break;
    case State::RECAL_DONE:
        state = State::IDLE;
        sendCommand(Command::CALIBRATE_END, 99U);
        break;
    }
}

/**
 * @brief Determines whether both desk nodes report an idle-compatible status.
 *
 * @return `true` if both nodes report an idle-compatible status, `false` otherwise.
 */
bool DeskService::isIdle() const
{
    return (stateA == 0U || stateA == 0x25U || stateA == 0x60U) && (stateB == 0U || stateB == 0x25U || stateB == 0x60U);
}

/**
 * @brief Begins movement preparation when a target is pending and the desk is idle; otherwise maintains the idle
 * command.
 */
void DeskService::handleStateIdle()
{
    if (pending && isIdle())
    {
        state = State::PREPARE;
    }
    else
    {
        sendCommand(Command::IDLE);
    }
}

/**
 * @brief Determines the movement direction and prepares the desk to move toward the target.
 *
 * Adjusts the target within the configured limits when necessary, sends the pre-movement
 * command, or clears the pending request when the target is already within range.
 */
void DeskService::handleStatePrepare()
{
    if (encoderTarget < getEncoderMin())
    {
        if (encoderTarget >= Encoder::minLimit + Encoder::targetOffset)
        {
            encoderTarget -= Encoder::targetOffset;
        }
        state = State::DOWN;
    }
    else if (encoderTarget > getEncoderMax())
    {
        if (encoderTarget <= Encoder::maxLimit - Encoder::targetOffset)
        {
            encoderTarget += Encoder::targetOffset;
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
    sendCommand(Command::PRE_MOVE);
}

/**
 * @brief Continues lowering the desk until the target is reached or movement times out.
 *
 * Transitions to the stop state when either encoder reaches the target or the encoder response timeout expires.
 */
void DeskService::handleStateDown()
{
    if (encoderTarget >= getEncoderMin() || millis() - lastMillis > (0b1U << 8U))
    {
        state = State::STOP;
    }
    else
    {
        sendCommand(Command::LOWER);
    }
}

/**
 * @brief Continues raising the desk until the target is reached or encoder updates time out.
 */
void DeskService::handleStateUp()
{
    if (encoderTarget <= getEncoderMax() || millis() - lastMillis > (0b1U << 8U))
    {
        state = State::STOP;
    }
    else
    {
        sendCommand(Command::RAISE);
    }
}

/**
 * @brief Completes a movement operation once both desk nodes are idle.
 */
void DeskService::handleStateDone()
{
    if (isIdle())
    {
        pending = false;
        state = State::IDLE;
    }
    else
    {
        sendCommand(Command::FINISH);
    }
}

/**
 * @brief Advances the ongoing recalibration process.
 *
 * Transitions to the recalibration-complete state when both nodes are ready
 * and the maximum encoder reading is within the calibration limit; otherwise,
 * continues recalibration.
 */
void DeskService::handleStateRecalOngoing()
{
    if (stateA == 1U && stateB == 1U && getEncoderMax() <= 99U)
    {
        state = State::RECAL_DONE;
    }
    else
    {
        sendCommand(Command::CALIBRATE_BEGIN, 0U);
    }
}

/**
 * @brief Sends a movement command with the target constrained to a safe range.
 *
 * @param command Movement command to send.
 */
void DeskService::sendCommand(Command command)
{
    const unsigned int maxCurrent{getEncoderMax()};
    const unsigned int minCurrent{getEncoderMin()};
    const unsigned int maxTarget{minCurrent < Encoder::maxLimit - Encoder::maxDelta ? minCurrent + Encoder::maxDelta
                                                                                    : Encoder::maxLimit};
    const unsigned int minTarget{maxCurrent > Encoder::minLimit + Encoder::maxDelta ? maxCurrent - Encoder::maxDelta
                                                                                    : Encoder::minLimit};
    sendCommand(command, constrain(encoderTarget, minTarget, maxTarget));
}

/**
 * @brief Sends a command and position payload over the LIN interface.
 *
 * @param command Command code to transmit.
 * @param position Position value included in the command payload.
 */
void DeskService::sendCommand(Command command, unsigned int position)
{
    for (unsigned char idx{0U}; idx < 6U; ++idx)
    {
        lin.send(0x10U);
    }
    lin.send(0x1U);
    const unsigned char packet[3U]{static_cast<unsigned char>(position & 0xFFU),
                                   static_cast<unsigned char>(position >> 8U),
                                   static_cast<unsigned char>(command)};
    lin.send(0x12U, packet);
}

/**
 * @brief Generates a square-wave tone at the specified frequency.
 *
 * @param frequency Tone frequency in hertz.
 */
void DeskService::tone(unsigned int frequency)
{
    if (frequency != 0U)
    {
        const unsigned int halfperiod{static_cast<unsigned int>(500'000UL / frequency)};
        const unsigned int delay{static_cast<unsigned int>(halfperiod - (48'000'000UL / F_CPU))};
        for (unsigned long idx{0UL}; idx < (0b1UL << 17U) / halfperiod; ++idx)
        {
            digitalWrite(Pin::tone, HIGH);
            delayMicroseconds(delay);
            digitalWrite(Pin::tone, LOW);
            delayMicroseconds(delay);
        }
    }
}

/**
 * @brief Starts desk recalibration when both desk nodes are idle.
 */
void DeskService::recalibrate()
{
    if (isIdle())
    {
        tone(0b1U << 12U);
        pending = false;
        state = State::RECAL_PREPARE;
    }
}

/**
 * @brief Stores a new upper desk-height preset.
 *
 * @param preset Upper desk-height preset to store.
 */
void DeskService::setPresetHigh(unsigned int preset)
{
    if (preset != presetHigh && preset <= Encoder::maxLimit && preset >= Encoder::minLimit)
    {
        presetHigh = preset;
        EEPROM.put(static_cast<int>('h'), presetHigh);
        console.send('h', presetHigh);
        return;
    }
    console.send('H', preset);
}

/**
 * @brief Stores and reports a new lower desk preset.
 *
 * Zero and unchanged preset values are ignored.
 *
 * @param preset Lower desk position to store.
 */
void DeskService::setPresetLow(unsigned int preset)
{
    if (preset != presetLow && preset <= Encoder::maxLimit && preset >= Encoder::minLimit)
    {
        presetLow = preset;
        EEPROM.put(static_cast<int>('l'), presetLow);
        console.send('l', presetLow);
        return;
    }
    console.send('L', preset);
}

/**
 * @brief Sets a valid target position and marks movement as pending.
 *
 * @param position Target position to move the desk to. Zero and `0xFFFF` are ignored.
 */
void DeskService::setTarget(unsigned int position)
{
    if (position != 0U && position != 0xFFFFU)
    {
        encoderTarget = position;
        pending = true;
        console.send('p', encoderTarget);
        return;
    }
    console.send('P', position);
}

/**
 * @brief Gets the greater of the two encoder values.
 *
 * @return unsigned int The greater encoder value.
 */
unsigned int DeskService::getEncoderMax() const { return encoderA > encoderB ? encoderA : encoderB; }

/**
 * @brief Gets the smaller current encoder value.
 *
 * @return unsigned int The lower value reported by the encoder nodes.
 */
unsigned int DeskService::getEncoderMin() const { return encoderA < encoderB ? encoderA : encoderB; }

/**
 * @brief Retrieves the configured high preset height.
 *
 * @return unsigned int The stored high preset value.
 */
unsigned int DeskService::getPresetHigh() const { return presetHigh; }

/**
 * @brief Retrieves the configured low preset.
 *
 * @return unsigned int The stored low preset value.
 */
unsigned int DeskService::getPresetLow() const { return presetLow; }

/**
 * @brief Gets the current desk service state.
 *
 * @return State Current movement or recalibration state.
 */
DeskService::State DeskService::getState() const { return state; }

/**
 * @brief Provides access to the singleton DeskService instance.
 *
 * @return DeskService& Reference to the shared DeskService instance.
 */
DeskService &DeskService::getInstance()
{
    static DeskService instance;
    return instance;
}

DeskService &desk{DeskService::getInstance()}; // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

#endif // ARDUINO_ARCH_AVR
