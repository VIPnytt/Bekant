#pragma once

#ifdef ARDUINO_ARCH_AVR

class ConsoleHandler
{
private:
    char buffer[5U]{'\n'};

    unsigned char length{0U};

    void process();

    unsigned int parseDigits();

public:
    void handle();
};

#endif // ARDUINO_ARCH_AVR
