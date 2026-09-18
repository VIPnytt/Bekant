#pragma once

#ifdef ARDUINO_ARCH_ESP32

#include <cstdint>

class PresetHandler
{
private:
    bool saved{true};

    uint16_t high{0U};
    uint16_t low{0U};

    unsigned long lastMillis{0UL};

public:
    void begin();

    void handle();

    void setHigh();

    void setHigh(float height);

    void setHigh(uint16_t encoder);

    void setLow();

    void setLow(float height);

    void setLow(uint16_t encoder);

    [[nodiscard]] float getHigh() const;

    [[nodiscard]] float getLow() const;
};

#endif // ARDUINO_ARCH_ESP32
