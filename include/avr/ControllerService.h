#pragma once

#ifdef ARDUINO_ARCH_AVR

#include "avr/ButtonHandler.h"
#include "avr/ConsoleHandler.h"
#include "avr/LegHandler.h"

class ControllerService
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

    static ControllerService &getInstance();

private:
    bool pending{false};

    unsigned char state8{0U};
    unsigned char state9{0U};

    unsigned int encoder8{0U};
    unsigned int encoder9{0U};
    unsigned int encoderTarget{0U};
    unsigned int presetHigh{0xFFFFU};
    unsigned int presetLow{0xFFFFU};

    unsigned long lastMillis{0U};

    ButtonHandler button{};

    ConsoleHandler console{};

    /**
 * Handles leg movement commands and encoder communication.
 */
LegHandler lin{};

    State state{State::IDLE};

    /**
     * Reads incoming desk communication data.
     *
     * @return `true` if data is received, `false` otherwise.
     */
    [[nodiscard]] bool read();

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
    void sendCommand(LegHandler::Command command);

    /**
     * Determines whether the desk service is idle.
     *
     * @return `true` if the service is idle, `false` otherwise.
     */
    [[nodiscard]] bool isIdle() const;
};

// NOLINTNEXTLINE(bugprone-dynamic-static-initializers,cppcoreguidelines-avoid-non-const-global-variables)
extern ControllerService &controller;

#endif // ARDUINO_ARCH_AVR
