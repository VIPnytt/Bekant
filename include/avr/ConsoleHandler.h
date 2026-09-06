#pragma once

#ifdef ARDUINO_ARCH_AVR

/**
 * Handles buffered console input.
 */
class ConsoleHandler
{
private:
    char buffer[5U]{0};

    unsigned char length{0U};

    void process();

    unsigned int parseDigits();

public:
    enum class Command : unsigned char
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
        TONE,
        VERSION,
    };

    void handle();
    void print(Command command, const char *text);
    void print(Command command, unsigned int value);
    void write(Command command);
    void write(Command command, unsigned char byte);
    void write(Command command, unsigned char byte1, unsigned char byte2);
    void write(Command command, unsigned char byte1, unsigned char byte2, unsigned char byte3);
};

#endif // ARDUINO_ARCH_AVR
