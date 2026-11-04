#pragma once

#ifdef ARDUINO_ARCH_AVR

class TimeHandler
{
public:
    static void begin();

    /**
     * Returns the elapsed milliseconds since Timer0 initialization.
     */
    [[nodiscard]] static unsigned long millis();
};

#endif // ARDUINO_ARCH_AVR
