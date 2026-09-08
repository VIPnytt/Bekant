#pragma once

#ifdef ARDUINO_ARCH_AVR

#include <HardwareSerial.h>

/**
 * Handles buffered console input.
 */
class ConsoleHandler
{
public:
    enum class Command : unsigned char
    {
        CALIBRATE = 1U,
        POSITION,
        PRESET_HIGH,
        PRESET_LOW,
        TONE,
    };

    enum class State : unsigned char
    {
        BUTTON_DOWN = 1U,
        BUTTON_UP,
        ENCODER8,
        ENCODER9,
        INITIALIZE,
        NODE8,
        NODE9,
        POSITION,
        PRESET_HIGH,
        PRESET_LOW,
        STATE8,
        STATE9,
    };

    void handle();

    void send(State state);

    void send(State state, unsigned char byte);

    void send(State state, unsigned int value);

    template <unsigned int N> void send(State state, const unsigned char (&data)[N])
    {
        static_assert(N < (0b1U << 4U));
        Serial1.write((N << 4U) | static_cast<unsigned char>(state));
        Serial1.write(data, N);
    }

private:
    unsigned char lengthRx{0U};

    unsigned char bufferRx[0b1U << 4U]{};

    unsigned int bytesRx{0U};

    Command commandRx{};

    /**
     * Parses buffered console input into a command.
     */
    void parse();
};

#endif // ARDUINO_ARCH_AVR
