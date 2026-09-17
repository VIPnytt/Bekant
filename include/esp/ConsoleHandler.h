#pragma once

#ifdef ARDUINO_ARCH_ESP32

#include <span>
#include <string>

class ConsoleHandler
{
public:
    enum class Command : uint8_t // NOLINT(performance-enum-size)
    {
        POSITION,
        PRESET_HIGH,
        PRESET_LOW,
        RECALIBRATE,
        TONE,
    };

    enum class State : uint8_t // NOLINT(performance-enum-size)
    {
        BUTTONS,
        CONSOLE,
        INITIALIZATION,
        LIN,
        NODE8,
        NODE9,
        PRESET_HIGH,
        PRESET_LOW,
        RESET_REASON,
        VERSION,
    };

    /**
     * Initializes console handling.
     */
    void begin();

    /**
     * Processes available console input.
     */
    void handle();

    /**
     * Forwards buffered console data.
     */
    void forward();

    void send(Command command);

    static void send(Command command, uint16_t value);

    static void send(Command command, uint16_t value1, uint16_t value2);

private:
    size_t bytesRx{0U};
    size_t bytesTx{0U};
    size_t lengthRx{0U};
    size_t lengthTx{0U};

    std::array<uint8_t, 0b1U << 4U> bufferRx{0U};
    std::array<uint8_t, 0b1U << 4U> bufferTx{0U};

    /**
     * Command to transmit.
     */
    Command commandTx{};

    /**
     * State received from the console.
     */
    State stateRx{};

    /**
     * Parses a received console payload.
     */
    void parse();

    static void write(std::span<const uint8_t> payload);
};

#endif // ARDUINO_ARCH_ESP32
