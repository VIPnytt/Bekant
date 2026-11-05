#pragma once

#ifdef ARDUINO_ARCH_AVR

class TimeHandler
{
public:
    static void begin();

    /**
     * Returns the Timer0 compare-match count in nominal milliseconds. The counter wraps on overflow and is not
     * reset by begin().
     */
    [[nodiscard]] static unsigned long millis();
};

#endif // ARDUINO_ARCH_AVR
