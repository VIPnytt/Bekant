#pragma once

#ifdef ARDUINO_ARCH_AVR

class ToneHandler
{
private:
    static constexpr unsigned char overhead{static_cast<unsigned char>(48'000'000UL / F_CPU)};

public:
    void begin();

    static void play(unsigned int frequency, unsigned long duration);
};

#endif // ARDUINO_ARCH_AVR
