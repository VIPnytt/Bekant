#pragma once

#ifdef ARDUINO_ARCH_AVR

#include "avr/LinHandler.h"

#include <stddef.h> // NOLINT(hicpp-deprecated-headers,modernize-deprecated-headers)

class DeskService
{
private:
    enum class Command : unsigned char
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

    enum class State : unsigned char
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
    char buttonCount{0};

    unsigned char nodeA[3U]{0U};
    unsigned char nodeB[3U]{0U};

    unsigned int bufferLength{0U};
    unsigned int encoderA{0U};
    unsigned int encoderB{0U};
    unsigned int encoderTarget{0};
    unsigned int presetHigh{0xFFFFU};
    unsigned int presetLow{0xFFFFU};

    unsigned long lastMillisButton{0U};
    unsigned long lastMillisEncoder{0U};

    LinHandler lin;

    State state{State::IDLE};

    void handleBuffer();
    void readButtons();
    void handleButtons();
    void handleEncoders();
    void handleStateIdle();
    void handleStatePrepare();
    void handleStateDown();
    void handleStateUp();
    void handleStateDone();
    void handleStateRecalOngoing();
    void parseBuffer();
    void parseEncoders();
    void playTone(unsigned int frequency);
    void sendCommand(Command command);
    void sendCommand(Command command, unsigned int target);
    void targetLower();
    void targetRise();

    bool isIdle();

    unsigned char sendPacket(unsigned char byte1, unsigned char byte2, unsigned char byte3, unsigned char byte4);

    unsigned int parseDigits();

public:
    void begin();
    void handle();

    static DeskService &getInstance();
};

// NOLINTNEXTLINE(bugprone-dynamic-static-initializers,cppcoreguidelines-avoid-non-const-global-variables)
extern DeskService &desk;

#endif // ARDUINO_ARCH_AVR
