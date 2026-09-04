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
    void setTarget(unsigned int position);
    void tone(unsigned int frequency);

    [[nodiscard]] unsigned int getEncoderMax() const;
    [[nodiscard]] unsigned int getEncoderMin() const;
    [[nodiscard]] unsigned int getPresetHigh() const;
    [[nodiscard]] unsigned int getPresetLow() const;

    [[nodiscard]] State getState() const;

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

    unsigned char stateA{0U};
    unsigned char stateB{0U};

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

    /**
     * Reads incoming desk communication data.
     */
    void read();

    /**
     * Processes the current desk service state.
     */
    void process();

    /**
     * Handles the idle state.
     */
    void handleStateIdle();

    /**
     * Handles preparation before desk movement.
     */
    void handleStatePrepare();

    /**
     * Handles downward desk movement.
     */
    void handleStateDown();

    /**
     * Handles upward desk movement.
     */
    void handleStateUp();

    /**
     * Handles completion of a desk operation.
     */
    void handleStateDone();

    /**
     * Handles an ongoing recalibration.
     */
    void handleStateRecalOngoing();

    /**
     * Sends a desk command.
     *
     * @param command Command to send.
     */
    void sendCommand(Command command);

    /**
     * Sends a desk command with a target encoder position.
     *
     * @param command Command to send.
     * @param position Encoder position associated with the command.
     */
    void sendCommand(Command command, unsigned int position);

    /**
     * Determines whether the desk service is idle.
     *
     * @return `true` if the service is idle, `false` otherwise.
     */
    [[nodiscard]] bool isIdle();

    /**
     * Sends a four-byte packet.
     *
     * @param byte1 First packet byte.
     * @param byte2 Second packet byte.
     * @param byte3 Third packet byte.
     * @param byte4 Fourth packet byte.
     * @return `true` if a response byte is received, `false` otherwise.
     */
    bool sendPacket(unsigned char byte1, unsigned char byte2, unsigned char byte3, unsigned char byte4);
};

// NOLINTNEXTLINE(bugprone-dynamic-static-initializers,cppcoreguidelines-avoid-non-const-global-variables)
extern DeskService &desk;

#endif // ARDUINO_ARCH_AVR
