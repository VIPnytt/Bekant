#pragma once

#ifdef ARDUINO_ARCH_ESP32

#include <cstdint>
#include <utility>

class LegHandler
{
private:
    unsigned long lastMillis{0UL};

    bool saved{true};

    uint8_t state8{0U};
    uint8_t state9{0U};

    uint16_t encoder8{0U};
    uint16_t encoder9{0U};

public:
    void begin();

    void handle();

    void setNode8(uint16_t position, uint8_t state);

    void setNode9(uint16_t position, uint8_t state);

    [[nodiscard]] bool getIdle() const;

    [[nodiscard]] std::pair<float, float> getLegs() const;

    [[nodiscard]] std::pair<uint8_t, uint8_t> getStates() const;

    [[nodiscard]] std::pair<uint16_t, uint16_t> getEncoders() const;
};

#endif // ARDUINO_ARCH_ESP32
