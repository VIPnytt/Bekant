#pragma once

#ifdef ARDUINO_ARCH_AVR

/**
 * Handles buffered console input.
 */
class ConsoleHandler
{
private:
    char buffer[5U]{0};

    unsigned char commandRx{0U};
    unsigned char lengthRx{0U};

    unsigned char bufferRx[0b1U << 4U]{0U};

    unsigned int bytesRx{0U};

    void parse();

public:
    enum class Command : unsigned char
    {
        BUTTON_DOWN,
        BUTTON_UP,
        CALIBRATE,
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
    };

    void handle();
    void print(Command command, unsigned int value);
    void write(Command command);
    void write(Command command, unsigned char byte);
    void write(Command command, unsigned char byte1, unsigned char byte2);
    void write(Command command, unsigned char byte1, unsigned char byte2, unsigned char byte3);
};

#endif // ARDUINO_ARCH_AVR
