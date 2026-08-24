#pragma once

#ifdef ARDUINO_ARCH_AVR

#include "avr/LinHandler.h"

#include <stddef.h> // NOLINT(hicpp-deprecated-headers,modernize-deprecated-headers)

class DeskService
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

    char buffer[5U]{'\n'};

    int8_t buttonCount{0};

    uint16_t encoderA{0U};
    uint16_t encoderB{0U};
    uint16_t encoderTarget{0U};
    uint16_t presetHigh{0xFFFFU};
    uint16_t presetLow{0xFFFFU};

    unsigned long lastMillisButton{0U};
    unsigned long lastMillisEncoder{0U};

    size_t bufferLength{0U};

    LinHandler lin;

    State state{State::IDLE};

    void handleBuffer();
    void readButtons();
    void handleButtons();
    void handleEncoders();
    void parseEncoders(uint8_t nodeA, uint8_t nodeB);
    void sendCommand(Command command);
    void sendCommand(Command command, uint16_t target);
    void parseBuffer();
    void playTone(uint16_t frequency);
    void savePreset(char preset, uint16_t value);

    uint8_t sendPacket(uint8_t payload1, uint8_t payload2, uint8_t payload3, uint8_t payload4);

    uint16_t parseDigits();

public:
    void begin();
    void handle();
};

#endif // ARDUINO_ARCH_AVR
