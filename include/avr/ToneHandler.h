#pragma once

#ifdef ARDUINO_ARCH_AVR

class ToneHandler
{
private:
    static inline volatile bool active{false};

    static inline unsigned int durationMillis{0U};

    static inline unsigned long startMillis{0UL};

public:
    static void begin();

    static void handle();

    static void play(unsigned int frequency, unsigned int duration);

    static void stop();
};

#endif // ARDUINO_ARCH_AVR
