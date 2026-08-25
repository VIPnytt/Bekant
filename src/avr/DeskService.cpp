#ifdef ARDUINO_ARCH_AVR

#include "avr/DeskService.h"

#include "avr/constants.h"

#include <EEPROM.h>
#include <HardwareSerial.h>
#include <wiring.h>

void DeskService::begin()
{
    Serial1.begin(115'200UL);
    delay(0b1U << 11U);
    pinMode(Pin::tone, OUTPUT);
    pinMode(Pin::buttonDown, INPUT_PULLUP);
    pinMode(Pin::buttonUp, INPUT_PULLUP);
    EEPROM.get<unsigned int>(static_cast<int>('h'), presetHigh);
    EEPROM.get<unsigned int>(static_cast<int>('l'), presetLow);
    Serial1.flush();
    if (presetHigh != 0xFFFFU)
    {
        Serial1.printf("h%u\n", presetHigh);
    }
    if (presetLow != 0xFFFFU)
    {
        Serial1.printf("l%u\n", presetLow);
    }
    Serial1.flush();
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
    signed char pid{-1};
    unsigned char errorCount{0U};
    for (size_t idx{0U}; idx < sizeof(data) / sizeof(data[0U]); ++idx)
    {
        if (idx == 4U || idx == 11U)
        {
            while (pid < 8)
            {
                ++pid;
                if (sendPacket(pid, data[idx][1U], data[idx][2U], data[idx][3U]) != 0U)
                {
                    break;
                }
            }
            if (pid >= 8)
            {
                ++errorCount;
            }
        }
        else if (idx == 18U)
        {
            while (pid < 8)
            {
                ++pid;
                sendPacket(pid, data[idx][1U], data[idx][2U], data[idx][3U]);
            }
        }
        else
        {
            sendPacket(data[idx][0U] == 0U ? pid : data[idx][0U], data[idx][1U], data[idx][2U], data[idx][3U]);
        }
    }
    constexpr unsigned char magicPacket[3U]{0xF6U, 0xFFU, 0xBFU};
    lin.send(0x12U, magicPacket);
    if (errorCount != 0U)
    {
        playTone(0b1U << 8U);
        Serial1.printf("I%u\n", errorCount);
    }
}

unsigned char DeskService::sendPacket(unsigned char byte1, unsigned char byte2, unsigned char byte3,
                                      unsigned char byte4)
{
    const unsigned char packet[8U]{byte1, byte2, byte3, byte4, 0xFFU, 0xFFU, 0xFFU, 0xFFU};
    lin.send(0x3C, packet);
    delay(sizeof(packet));
    unsigned char response[sizeof(packet)]{};
    return lin.request(0x3DU, response);
}

void DeskService::handle()
{
    handleEncoders();
    if (state != State::RECAL_PREPARE && state != State::RECAL_ONGOING && state != State::RECAL_DONE)
    {
        handleBuffer();
        handleButtons();
    }
}

void DeskService::readButtons()
{
    const bool _buttonDown{digitalRead(Pin::buttonDown) == LOW};
    const bool _buttonUp{digitalRead(Pin::buttonUp) == LOW};
    if (_buttonDown != buttonDown)
    {
        buttonDown = _buttonDown;
        if (buttonDown)
        {
            lastMillisButton = millis();
            --buttonCount;
        }
        Serial1.printf("d%u\n", static_cast<unsigned char>(buttonDown));
    }
    if (_buttonUp != buttonUp)
    {
        buttonUp = _buttonUp;
        if (buttonUp)
        {
            lastMillisButton = millis();
            ++buttonCount;
        }
        Serial1.printf("u%u\n", static_cast<unsigned char>(buttonUp));
    }
}

void DeskService::handleButtons()
{
    readButtons();
    if (buttonDown && buttonUp && millis() - lastMillisButton > 0b1U << 13U)
    {
        buttonCount = 0;
        state = State::RECAL_PREPARE;
    }
    else if (buttonDown && !buttonUp && millis() - lastMillisButton > 0b1U << 9U)
    {
        buttonCount = 0;
        targetLower();
    }
    else if (buttonUp && !buttonDown && millis() - lastMillisButton > 0b1U << 9U)
    {
        buttonCount = 0;
        targetRise();
    }
    else if (buttonCount == -2 && millis() - lastMillisButton > 0b1U << 8U)
    {
        buttonCount = 0;
        presetLow = max(encoderA, encoderB);
        EEPROM.put(static_cast<int>('l'), presetLow);
    }
    else if (buttonCount == -1 && presetLow != 0xFFFFU && millis() - lastMillisButton > 0b1U << 8U)
    {
        buttonCount = 0;
        encoderTarget = presetLow;
        move = true;
    }
    else if (buttonCount == 1 && presetHigh != 0xFFFFU && millis() - lastMillisButton > 0b1U << 8U)
    {
        buttonCount = 0;
        encoderTarget = presetHigh;
        move = true;
    }
    else if (buttonCount == 2 && millis() - lastMillisButton > 0b1U << 8U)
    {
        buttonCount = 0;
        presetHigh = min(encoderA, encoderB);
        EEPROM.put(static_cast<int>('h'), presetHigh);
    }
    else if (buttonCount != 0 && millis() - lastMillisButton > 0b1U << 8U)
    {
        buttonCount = 0;
    }
}

void DeskService::handleBuffer()
{
    const int byte{Serial1.read()};
    if (byte == static_cast<int>('\n') && bufferLength != 0U)
    {
        if (bufferLength <= sizeof(buffer))
        {
            parseBuffer();
        }
        bufferLength = 0U;
    }
    else if (byte != -1 && byte != static_cast<int>('\n'))
    {
        if (bufferLength < sizeof(buffer))
        {
            buffer[bufferLength] = static_cast<unsigned char>(byte);
        }
        ++bufferLength;
    }
}

void DeskService::parseBuffer()
{
    if (buffer[0U] == 'c')
    {
        playTone(0b1U << 10U);
        state = State::RECAL_PREPARE;
    }
    else if (buffer[0U] == 'e')
    {
        const unsigned int _target{parseDigits()};
        if (_target != 0U)
        {
            encoderTarget = _target;
            move = true;
        }
    }
    else if (buffer[0U] == 'h' && bufferLength == 1U && presetHigh != 0xFFFFU)
    {
        encoderTarget = presetHigh;
        move = true;
    }
    else if (buffer[0U] == 'h' && bufferLength != 1U)
    {
        const unsigned int _high{parseDigits()};
        if (_high != 0U && _high != presetHigh)
        {
            presetHigh = _high;
            EEPROM.put(static_cast<int>(buffer[0U]), presetHigh);
        }
    }
    else if (buffer[0U] == 'l' && bufferLength == 1U && presetLow != 0xFFFFU)
    {
        encoderTarget = presetLow;
        move = true;
    }
    else if (buffer[0U] == 'l' && bufferLength != 1U)
    {
        const unsigned int _low{parseDigits()};
        if (_low != 0U && _low != presetLow)
        {
            presetLow = _low;
            EEPROM.put(static_cast<int>(buffer[0U]), presetLow);
        }
    }
    else if (buffer[0U] == 't')
    {
        const unsigned int _frequency{parseDigits()};
        if (_frequency != 0U)
        {
            playTone(_frequency);
        }
    }
}

unsigned int DeskService::parseDigits()
{
    unsigned int value{0U};
    for (size_t idx{1U}; idx < bufferLength; ++idx)
    {
        if (buffer[idx] < '0' || buffer[idx] > '9')
        {
            return 0U;
        }
        value *= 10U;
        value += static_cast<unsigned int>(buffer[idx] - '0');
    }
    return value;
}

void DeskService::targetLower()
{
    const unsigned int current{max(encoderA, encoderB)};
    encoderTarget = current > (0b1U << 8U) ? current - (0b1U << 8U) : (0b1U << 8U);
    move = true;
}

void DeskService::targetRise()
{
    const unsigned int current{min(encoderA, encoderB)};
    encoderTarget = current < (0b1U << 13U) ? current + (0b1U << 8U) : (0b1U << 13U);
    move = true;
}

void DeskService::handleEncoders()
{
    constexpr unsigned char empty[3U]{0U, 0U, 0U};
    lin.send(0x11U, empty);
    const unsigned char charsA{lin.request(0x8U, nodeA)};
    const unsigned char charsB{lin.request(0x9U, nodeB)};
    const unsigned int _encoderA{static_cast<unsigned int>(nodeA[0U]) | static_cast<unsigned int>(nodeA[1U] << 8U)};
    const unsigned int _encoderB{static_cast<unsigned int>(nodeB[0U]) | static_cast<unsigned int>(nodeB[1U] << 8U)};
    if (_encoderA != encoderA)
    {
        if (charsA != static_cast<unsigned char>(sizeof(nodeA) + 1ULL))
        {
            Serial1.printf("A%u\n", _encoderA);
            if (move)
            {
                playTone(0b1U << 12U);
            }
            return;
        }
        encoderA = _encoderA;
        lastMillisEncoder = millis();
        Serial1.printf("a%u\n", encoderA);
    }
    if (_encoderB != encoderB)
    {
        if (charsB != static_cast<unsigned char>(sizeof(nodeB) + 1ULL))
        {
            Serial1.printf("B%u\n", _encoderB);
            if (move)
            {
                playTone(0b1U << 12U);
            }
            return;
        }
        encoderB = _encoderB;
        lastMillisEncoder = millis();
        Serial1.printf("b%u\n", encoderB);
    }
    parseEncoders();
}

void DeskService::parseEncoders()
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

bool DeskService::isIdle()
{
    return (nodeA[2U] == 0U || nodeA[2U] == 0x25U || nodeA[2U] == 0x60U) &&
           (nodeB[2U] == 0U || nodeB[2U] == 0x25U || nodeB[2U] == 0x60U);
}

void DeskService::handleStateIdle()
{
    if (move && isIdle())
    {
        state = State::PREPARE;
        return;
    }
    sendCommand(Command::IDLE);
}

void DeskService::handleStatePrepare()
{
    if (encoderTarget < min(encoderA, encoderB))
    {
        if (encoderTarget > Encoder::minLimit + Encoder::targetOffset)
        {
            encoderTarget -= Encoder::targetOffset;
        }
        state = State::DOWN;
    }
    else if (encoderTarget > max(encoderA, encoderB))
    {
        if (encoderTarget < Encoder::maxLimit - Encoder::targetOffset)
        {
            encoderTarget += Encoder::targetOffset;
        }
        state = State::UP;
    }
    else
    {
        move = false;
        state = State::IDLE;
        return;
    }
    lastMillisEncoder = millis();
    sendCommand(Command::PRE_MOVE);
}

void DeskService::handleStateDown()
{
    if (encoderTarget >= min(encoderA, encoderB) || millis() - lastMillisEncoder > (0b1U << 8U))
    {
        state = State::STOP;
        return;
    }
    sendCommand(Command::LOWER);
}

void DeskService::handleStateUp()
{
    if (encoderTarget <= max(encoderA, encoderB) || millis() - lastMillisEncoder > (0b1U << 8U))
    {
        state = State::STOP;
        return;
    }
    sendCommand(Command::RAISE);
}

void DeskService::handleStateDone()
{
    if (isIdle())
    {
        move = false;
        state = State::IDLE;
        return;
    }
    sendCommand(Command::FINISH);
}

void DeskService::handleStateRecalOngoing()
{
    if (nodeA[2U] == 1U && nodeB[2U] == 1U && max(encoderA, encoderB) <= 99U)
    {
        state = State::RECAL_DONE;
        return;
    }
    sendCommand(Command::CALIBRATE_BEGIN, 0U);
}

void DeskService::sendCommand(Command command)
{
    const unsigned int encoderMax{static_cast<unsigned int>(max(encoderA, encoderB))};
    const unsigned int encoderMin{static_cast<unsigned int>(min(encoderA, encoderB))};
    sendCommand(command,
                constrain(static_cast<unsigned int>(encoderTarget),
                          encoderMax > Encoder::maxDelta ? max(encoderMax - Encoder::maxDelta, Encoder::minLimit)
                                                         : Encoder::minLimit,
                          encoderMin < Encoder::maxLimit - Encoder::maxDelta
                              ? min(encoderMin + Encoder::maxDelta, Encoder::maxLimit)
                              : Encoder::maxLimit));
}

void DeskService::sendCommand(Command command, unsigned int payload)
{
    for (unsigned char idx{0U}; idx < 6U; ++idx)
    {
        lin.send(0x10U);
    }
    lin.send(0x1U);
    const unsigned char packet[3U]{static_cast<unsigned char>(payload & 0xFFU),
                                   static_cast<unsigned char>(payload >> 8U),
                                   static_cast<unsigned char>(command)};
    lin.send(0x12U, packet);
}

void DeskService::playTone(unsigned int frequency)
{
    const unsigned int halfperiod{static_cast<unsigned int>(500'000UL / frequency)};
    const unsigned int delay{static_cast<unsigned int>(halfperiod - (48'000'000UL / F_CPU))};
    for (uint32_t idx{0U}; idx < (0b1UL << 17U) / halfperiod; ++idx)
    {
        digitalWrite(Pin::tone, HIGH);
        delayMicroseconds(delay);
        digitalWrite(Pin::tone, LOW);
        delayMicroseconds(delay);
    }
}

DeskService &DeskService::getInstance()
{
    static DeskService instance;
    return instance;
}

DeskService &desk{DeskService::getInstance()}; // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

#endif // ARDUINO_ARCH_AVR
