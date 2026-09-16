#pragma once

#ifdef ARDUINO_ARCH_AVR

class ToneHandler
{
private:
    static constexpr unsigned char overhead{static_cast<unsigned char>(48'000'000UL / F_CPU)};

    static constexpr unsigned char minFrequency{static_cast<unsigned char>((500'000UL / ((0b1UL << 16U) - 1UL)) + 1UL)};

public:
    void begin();

    static void play(unsigned int frequency, unsigned int duration);
};

#endif // ARDUINO_ARCH_AVR
