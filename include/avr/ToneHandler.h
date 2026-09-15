#pragma once

#ifdef ARDUINO_ARCH_AVR

class ToneHandler
{
private:
    static constexpr unsigned char overhead{static_cast<unsigned char>(48'000'000UL / F_CPU)};

    static constexpr unsigned int maxFrequency{static_cast<unsigned int>(500'000UL / (overhead + 1UL))};

    static constexpr unsigned int minFrequency{static_cast<unsigned int>((500'000UL / ((0b1UL << 16U) - 1UL)) + 1UL)};

public:
    void begin();

    static void play(unsigned int frequency, unsigned long duration);
};

#endif // ARDUINO_ARCH_AVR
