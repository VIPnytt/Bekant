#pragma once

#ifdef ARDUINO_ARCH_AVR

#include <HardwareSerial.h>

/**
 * Handles buffered console input.
 */
class ConsoleHandler
{
public:
    /**
     * Identifies protocol message commands exchanged with the console.
     */
    enum class Command : unsigned char
    {
        CALIBRATE = 1U,
        POSITION,
        PRESET_HIGH,
        PRESET_LOW,
        TONE,
    };

    /**
     * Identifies protocol message states exchanged with the console.
     */
    enum class State : unsigned char
    {
        BUTTON_DOWN = 1U,
        BUTTON_UP,
        CONSOLE,
        INITIALIZATION,
        LIN,
        NODE8,
        NODE9,
        PRESET_HIGH,
        PRESET_LOW,
        VERSION,
    };

    /**
     * Processes buffered console input.
     */
    void handle();

    /**
     * Sends a protocol state without a payload.
     * @param state State to send.
     */
    void send(State state);

    /**
     * Sends a protocol state with an 8-bit payload.
     * @param state State to send.
     * @param byte 8-bit payload.
     */
    void send(State state, unsigned char byte);

    /**
     * Sends a protocol state with a 16-bit payload.
     * @param state State to send.
     * @param value 16-bit payload.
     */
    void send(State state, unsigned int value);

    /**
     * Sends a protocol state with a fixed-size byte payload.
     * @param state State to send.
     * @param data Byte payload to send.
     * @tparam N Number of bytes in the payload; must be fewer than 16.
     */
    template <unsigned int N> void send(State state, const unsigned char (&data)[N])
    {
        static_assert(N < (0b1U << 4U));
        Serial1.write((N << 4U) | static_cast<unsigned char>(state));
        Serial1.write(data, N);
    }

private:
    unsigned char errors{0U};
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
