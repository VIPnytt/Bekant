#pragma once

#ifdef __AVR__

#include <HardwareSerial.h>

/**
 * Handles buffered console input.
 */
class ConsoleHandler
{
private:
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

    unsigned char errors{0U};
    unsigned char lengthRx{0U};

    unsigned char bufferRx[0b1U << 4U]{};

    unsigned int bytesRx{0U};

    Command commandRx{};

    /**
     * Parses buffered console input into a command.
     */
    void parse();

public:
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
        POSITION,
        PRESET_HIGH,
        PRESET_LOW,
        VERSION,
    };

    void begin();

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
#ifdef __AVR_ATtiny841__
        Serial1.write(static_cast<unsigned char>((N << 4U) | static_cast<unsigned char>(state)));
        Serial1.write(data, N);
#elif defined(__AVR_ATtiny1624__)
        Serial.write(static_cast<unsigned char>((N << 4U) | static_cast<unsigned char>(state)));
        Serial.write(data, N);
#endif // __AVR_ATtiny841__
    }

    static ConsoleHandler &getInstance();
};

// NOLINTNEXTLINE(bugprone-dynamic-static-initializers,cppcoreguidelines-avoid-non-const-global-variables)
extern ConsoleHandler &console;

#endif // __AVR__
