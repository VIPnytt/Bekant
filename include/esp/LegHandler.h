#pragma once

#ifdef ARDUINO_ARCH_ESP32

#include <cstdint>
#include <utility>

class LegHandler
{
private:
    unsigned long lastMillis{0UL};

    static inline bool saved{true};

    static inline uint8_t state8{0U};
    static inline uint8_t state9{0U};

    static inline uint16_t encoder8{0U};
    static inline uint16_t encoder9{0U};

    static void setStatus();

public:
    void begin();

    void handle();

    [[nodiscard]] std::pair<uint16_t, uint16_t> getEncoders() const;

    [[nodiscard]] std::pair<float, float> getLegs() const;

    [[nodiscard]] std::pair<uint8_t, uint8_t> getStates() const;

    static void setNode8(uint16_t position, uint8_t state);

    static void setNode9(uint16_t position, uint8_t state);
};

#endif // ARDUINO_ARCH_ESP32
