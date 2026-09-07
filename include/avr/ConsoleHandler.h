#pragma once

#ifdef ARDUINO_ARCH_AVR

/**
 * Handles buffered console input.
 */
class ConsoleHandler
{
public:
    enum class Command : unsigned char
    {
        CALIBRATE,
        POSITION,
        PRESET_HIGH,
        PRESET_LOW,
        TONE,
    };

    enum class State : unsigned char
    {
        BUTTON_DOWN,
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
    void print(State state, unsigned int value);
    void write(State state);
    void write(State state, unsigned char byte);
    void write(State state, unsigned char byte1, unsigned char byte2);
    void write(State state, unsigned char byte1, unsigned char byte2, unsigned char byte3);

private:
    unsigned char commandLength{0U};

    unsigned char commandBuffer[0b1U << 4U]{0U};

    unsigned int commandBytes{0U};

    Command command{};

    /**
     * Parses buffered console input into a command.
     */
    void parse();
};

#endif // ARDUINO_ARCH_AVR
