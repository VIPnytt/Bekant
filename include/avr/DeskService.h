#pragma once

#ifdef ARDUINO_ARCH_AVR

#include "avr/ButtonHandler.h"
#include "avr/ConsoleHandler.h"
#include "avr/LinHandler.h"

class DeskService
{
public:
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

    void begin();
    void handle();
    void recalibrate();
    void setPresetHigh(unsigned int preset);
    void setPresetLow(unsigned int preset);
    void setTarget(unsigned int target);
    void tone(unsigned int frequency);

    unsigned int getEncoderMax() const;
    unsigned int getEncoderMin() const;
    unsigned int getPresetHigh() const;
    unsigned int getPresetLow() const;

    State getState() const;

    static DeskService &getInstance();

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

    bool pending{false};

    unsigned char nodeA[3U]{0U};
    unsigned char nodeB[3U]{0U};

    unsigned int encoderA{0U};
    unsigned int encoderB{0U};
    unsigned int encoderTarget{0U};
    unsigned int presetHigh{0xFFFFU};
    unsigned int presetLow{0xFFFFU};

    unsigned long lastMillis{0U};

    ButtonHandler button{};

    ConsoleHandler console{};

    LinHandler lin{};

    State state{State::IDLE};

    void read();
    void handleStateIdle();
    void handleStatePrepare();
    void handleStateDown();
    void handleStateUp();
    void handleStateDone();
    void handleStateRecalOngoing();

    void process();

    void sendCommand(Command command);
    void sendCommand(Command command, unsigned int target);

    bool isIdle();

    unsigned char sendPacket(unsigned char byte1, unsigned char byte2, unsigned char byte3, unsigned char byte4);
};

// NOLINTNEXTLINE(bugprone-dynamic-static-initializers,cppcoreguidelines-avoid-non-const-global-variables)
extern DeskService &desk;

#endif // ARDUINO_ARCH_AVR
