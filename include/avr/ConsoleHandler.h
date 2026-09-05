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
    void handle();
    void send(char command, unsigned int value);
};

#endif // ARDUINO_ARCH_AVR
