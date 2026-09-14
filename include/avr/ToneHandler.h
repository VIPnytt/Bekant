#pragma once

#ifdef ARDUINO_ARCH_AVR

class ToneHandler
{
private:
    static constexpr unsigned char overhead{static_cast<unsigned char>(48'000'000UL / F_CPU)};

    static constexpr unsigned int maxFrequency{500'000UL / (overhead + 1U)};

    static constexpr unsigned int minFrequency{(500'000UL / ((0b1U << 16U) - 1U)) + 1U};

public:
    void begin();

    static void play(unsigned int frequency, unsigned long duration);
};

#endif // ARDUINO_ARCH_AVR
