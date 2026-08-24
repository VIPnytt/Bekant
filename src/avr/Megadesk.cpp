#ifdef ARDUINO_ARCH_AVR

#include "avr/Megadesk.h"

#include "avr/constants.h"

#include <EEPROM.h>
#include <HardwareSerial.h>
#include <wiring.h>

void Megadesk::begin()
{
    Serial1.begin(115'200UL);
    delay(0b1U << 8U);
    pinMode(Pin::tone, OUTPUT);
    pinMode(Pin::buttonDown, INPUT_PULLUP);
    pinMode(Pin::buttonUp, INPUT_PULLUP);
    EEPROM.get<uint16_t>(static_cast<int>('h'), presetHigh);
    EEPROM.get<uint16_t>(static_cast<int>('l'), presetLow);
    Serial1.flush();
    if (presetHigh != 0U && presetHigh != 0xFFFFU)
    {
        Serial1.printf("h%u\n", presetHigh);
    }
    if (presetLow != 0U && presetLow != 0xFFFFU)
    {
        Serial1.printf("l%u\n", presetLow);
    }
    lin.begin();
    constexpr uint8_t data[21U][4U]{
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
    int8_t pid{-1};
    uint8_t errorCount{0U};
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
    constexpr uint8_t magicPacket[3U]{0xF6U, 0xFFU, 0xBFU};
    lin.send(0x12U, magicPacket);
    if (errorCount != 0U)
    {
        playTone(0b1U << 11U);
        Serial1.printf("I%u\n", errorCount);
    }
}

uint8_t Megadesk::sendPacket(uint8_t payload1, uint8_t payload2, uint8_t payload3, uint8_t payload4)
{
    const uint8_t packet[8U]{payload1, payload2, payload3, payload4, 0xFFU, 0xFFU, 0xFFU, 0xFFU};
    lin.send(0x3C, packet);
    delay(sizeof(packet));
    uint8_t response[sizeof(packet)]{};
    return lin.request(0x3DU, response);
}

void Megadesk::handle()
{
    handleEncoders();
    if (state != State::RECAL_PREPARE && state != State::RECAL_ONGOING && state != State::RECAL_DONE)
    {
        handleBuffer();
        handleButtons();
    }
}

void Megadesk::handleButtons()
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
        else if (move)
        {
            encoderTarget = max(encoderA, encoderB) - (0b1U << 8U);
        }
        Serial1.printf("d%u\n", static_cast<uint8_t>(buttonDown));
    }
    if (_buttonUp != buttonUp)
    {
        buttonUp = _buttonUp;
        if (buttonUp)
        {
            lastMillisButton = millis();
            ++buttonCount;
        }
        else if (move)
        {
            encoderTarget = min(encoderA, encoderB) + (0b1U << 8U);
        }
        Serial1.printf("u%u\n", static_cast<uint8_t>(buttonUp));
    }
    if (buttonDown && buttonUp && millis() - lastMillisButton > 0b1U << 13U)
    {
        buttonCount = 0;
        playTone(0b1U << 10U);
        state = State::RECAL_PREPARE;
    }
    else if (buttonDown && !buttonUp && millis() - lastMillisButton > 0b1U << 9U)
    {
        buttonCount = 0;
        encoderTarget = max(encoderA, encoderB) - (0b1U << 8U);
        move = true;
    }
    else if (buttonUp && !buttonDown && millis() - lastMillisButton > 0b1U << 9U)
    {
        buttonCount = 0;
        encoderTarget = min(encoderA, encoderB) + (0b1U << 8U);
        move = true;
    }
    else if (buttonCount == -2 && millis() - lastMillisButton > 0b1U << 8U)
    {
        buttonCount = 0;
        presetLow = max(encoderA, encoderB);
        savePreset('l', presetLow);
        playTone(0b1U << 9U);
    }
    else if (buttonCount == -1 && millis() - lastMillisButton > 0b1U << 8U)
    {
        buttonCount = 0;
        if (presetLow != 0U && presetLow != 0xFFFFU)
        {
            Serial1.printf("l%u\n", presetLow);
            encoderTarget = presetLow;
        }
    }
    else if (buttonCount == 1 && millis() - lastMillisButton > 0b1U << 8U)
    {
        buttonCount = 0;
        if (presetHigh != 0U && presetHigh != 0xFFFFU)
        {
            Serial1.printf("h%u\n", presetHigh);
            encoderTarget = presetHigh;
        }
    }
    else if (buttonCount == 2 && millis() - lastMillisButton > 0b1U << 8U)
    {
        buttonCount = 0;
        presetHigh = min(encoderA, encoderB);
        savePreset('h', presetHigh);
        playTone(0b1U << 9U);
    }
    else if (buttonCount != 0 && millis() - lastMillisButton > 0b1U << 8U)
    {
        buttonCount = 0;
    }
}

void Megadesk::handleBuffer()
{
    static size_t idx{0U};
    static uint8_t data[5U]{0U};
    const int byte{Serial1.read()};
    if (byte != -1)
    {
        if (idx <= sizeof(data) && byte == static_cast<int>('\n'))
        {
            parseSerial(data);
            idx = 0U;
        }
        else if (idx < sizeof(data))
        {
            data[idx++] = static_cast<uint8_t>(byte);
        }
    }
}

void Megadesk::parseSerial(const uint8_t (&data)[1U])
{
    if (data[0U] == static_cast<uint8_t>('c'))
    {
        playTone(0b1U << 10U);
        state = State::RECAL_PREPARE;
    }
    else if (data[0U] == static_cast<uint8_t>('h'))
    {
        encoderTarget = presetHigh;
        move = true;
    }
    else if (data[0U] == static_cast<uint8_t>('l'))
    {
        encoderTarget = presetLow;
        move = true;
    }
}

void Megadesk::savePreset(char preset, uint16_t value)
{
    EEPROM.put(static_cast<int>(preset), value);
    Serial1.printf("%c%u\n", preset, value);
}

void Megadesk::handleEncoders()
{
    constexpr uint8_t empty[3U]{0U, 0U, 0U};
    lin.send(0x11U, empty);
    uint8_t nodeA[3U]{0U};
    uint8_t nodeB[3U]{0U};
    const uint8_t charsA{lin.request(0x8U, nodeA)};
    const uint8_t charsB{lin.request(0x9U, nodeB)};
    const uint16_t _encoderA{static_cast<uint16_t>(nodeA[0U]) | static_cast<uint16_t>(nodeA[1U] << 8U)};
    const uint16_t _encoderB{static_cast<uint16_t>(nodeB[0U]) | static_cast<uint16_t>(nodeB[1U] << 8U)};
    if (_encoderA != encoderA)
    {
        if (charsA != static_cast<uint8_t>(sizeof(nodeA) + 1ULL))
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
        if (charsB != static_cast<uint8_t>(sizeof(nodeB) + 1ULL))
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
    switch (state)
    {
    case State::IDLE:
        if (move && (nodeA[2U] == 0U || nodeA[2U] == 0x25U || nodeA[2U] == 0x60U) &&
            (nodeB[2U] == 0U || nodeB[2U] == 0x25U || nodeB[2U] == 0x60U))
        {
            state = State::PREPARE;
        }
        else
        {
            sendCommand(Command::IDLE);
        }
        break;
    case State::PREPARE:
        if (encoderTarget < min(encoderA, encoderB))
        {
            encoderTarget -= 137U;
            state = State::DOWN;
        }
        else if (encoderTarget > max(encoderA, encoderB))
        {
            encoderTarget += 137U;
            state = State::UP;
        }
        else
        {
            move = false;
            state = State::IDLE;
            break;
        }
        lastMillisEncoder = millis();
        sendCommand(Command::PRE_MOVE);
        break;
    case State::DOWN:
        if (encoderTarget >= min(encoderA, encoderB) || millis() - lastMillisEncoder > (0b1U << 8U))
        {
            state = State::STOP;
        }
        else
        {
            sendCommand(Command::LOWER);
        }
        break;
    case State::UP:
        if (encoderTarget <= max(encoderA, encoderB) || millis() - lastMillisEncoder > (0b1U << 8U))
        {
            state = State::STOP;
        }
        else
        {
            sendCommand(Command::RAISE);
        }
        break;

    case State::STOP:
        state = State::DONE;
        sendCommand(Command::OK);
        break;
    case State::DONE:
        if ((nodeA[2U] == 0U || nodeA[2U] == 0x25U || nodeA[2U] == 0x60U) &&
            (nodeB[2U] == 0U || nodeB[2U] == 0x25U || nodeB[2U] == 0x60U))
        {
            move = false;
            state = State::IDLE;
        }
        else
        {
            sendCommand(Command::FINISH);
        }
        break;
    case State::RECAL_PREPARE:
        state = State::RECAL_ONGOING;
        sendCommand(Command::PRE_MOVE);
        break;
    case State::RECAL_ONGOING:
        if (max(encoderA, encoderB) <= 99U && nodeA[2U] == 1U && nodeB[2U] == 1U)
        {
            state = State::RECAL_DONE;
        }
        else
        {
            sendCommand(Command::CALIBRATE_BEGIN, 0U);
        }
        break;
    case State::RECAL_DONE:
        state = State::IDLE;
        sendCommand(Command::CALIBRATE_END, 99U);
        break;
    }
}

void Megadesk::sendCommand(Command command)
{
    sendCommand(
        command,
        constrain(encoderTarget,
                  static_cast<uint16_t>(max(0b1U << 8U, static_cast<int>(max(encoderA, encoderB)) - (0b1U << 8U))),
                  static_cast<uint16_t>(min(encoderA, encoderB) + (0b1U << 8U))));
}

void Megadesk::sendCommand(Command command, uint16_t payload)
{
    for (uint8_t idx{0U}; idx < 6U; ++idx)
    {
        lin.send(0x10U);
    }
    lin.send(0x1U);
    const uint8_t packet[3U]{
        static_cast<uint8_t>(payload & 0xFFU), static_cast<uint8_t>(payload >> 8U), static_cast<uint8_t>(command)};
    lin.send(0x12U, packet);
}

void Megadesk::playTone(uint16_t frequency)
{
    const uint16_t halfperiod{static_cast<uint16_t>(500'000UL / frequency)};
    const uint16_t delay{static_cast<uint16_t>(halfperiod - (48'000'000UL / F_CPU))};
    for (uint32_t idx{0U}; idx < (0b1UL << 17U) / halfperiod; ++idx)
    {
        digitalWrite(Pin::tone, HIGH);
        delayMicroseconds(delay);
        digitalWrite(Pin::tone, LOW);
        delayMicroseconds(delay);
    }
}

#endif // ARDUINO_ARCH_AVR
