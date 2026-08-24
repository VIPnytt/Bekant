#pragma once

#ifdef ARDUINO_ARCH_AVR

#include "avr/Lin.h"

#include <stddef.h> // NOLINT(hicpp-deprecated-headers,modernize-deprecated-headers)

class Megadesk
{
private:
    enum class Command : uint8_t
    {
        FINISH = 0x84U,
        LOWER = 0x85U,
        RAISE = 0x86U,
        OK = 0x87U,
        CALIBRATE_END = 0xBCU,
        CALIBRATE_BEGIN = 0xBDU,
        PRE_MOVE = 0xC4U,
        IDLE = 0xFCU,
    };

    enum class State : uint8_t
    {
        IDLE,
        PREPARE,
        DOWN,
        UP,
        STOP,
        DONE,
        RECAL_PREPARE,
        RECAL_ONGOING,
        RECAL_DONE,
    };

    bool buttonDown{false};
    bool buttonUp{false};
    bool move{false};

    int8_t buttonCount{0};

    uint16_t encoderA{0U};
    uint16_t encoderB{0U};
    uint16_t encoderTarget{0U};
    uint16_t presetHigh{0U};
    uint16_t presetLow{0U};

    unsigned long lastMillisButton{0U};
    unsigned long lastMillisEncoder{0U};

    Lin lin;

    State state{State::IDLE};

    void handleBuffer();
    void handleButtons();
    void handleEncoders();
    void sendCommand(Command command);
    void sendCommand(Command command, uint16_t target);
    void parseSerial(const uint8_t (&data)[1U]);
    void playTone(uint16_t frequency);
    void savePreset(char preset, uint16_t value);

    uint8_t sendPacket(uint8_t payload1, uint8_t payload2, uint8_t payload3, uint8_t payload4);

    template <size_t N> void parseSerial(const uint8_t (&data)[N])
    {
        if (data[0U] == static_cast<uint8_t>('e'))
        {
            encoderTarget = parseDigits(data);
            move = true;
        }
        else if (data[0U] == static_cast<uint8_t>('h'))
        {
            const uint16_t _high{parseDigits(data)};
            if (_high != presetHigh)
            {
                presetHigh = _high;
                savePreset('h', presetHigh);
            }
        }
        else if (data[0U] == static_cast<uint8_t>('l'))
        {
            const uint16_t _low{parseDigits(data)};
            if (_low != presetLow)
            {
                presetLow = _low;
                savePreset('l', presetLow);
            }
        }
        else if (data[0U] == static_cast<uint8_t>('t'))
        {
            playTone(parseDigits(data));
        }
    }

    template <size_t N> uint16_t parseDigits(const uint8_t (&data)[N])
    {
        uint16_t value{0U};
        for (size_t idx{1U}; idx < N; ++idx)
        {
            value = static_cast<uint16_t>((value * 10U) + (data[idx] - static_cast<unsigned int>('0')));
        }
        return value;
    }

public:
    void begin();
    void handle();
};

#endif // ARDUINO_ARCH_AVR
