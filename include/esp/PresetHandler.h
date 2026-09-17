#pragma once

#ifdef ARDUINO_ARCH_ESP32

#include <cstdint>

class PresetHandler
{
private:
    static inline bool saved{true};

    static inline uint16_t high{0U};
    static inline uint16_t low{0U};

    static inline unsigned long lastMillis{0UL};

public:
    void begin();

    void handle();

    void setHigh(float height);

    void setLow(float height);

    [[nodiscard]] float getHigh() const;

    [[nodiscard]] float getLow() const;

    static void setHigh(uint16_t encoder);

    static void setLow(uint16_t encoder);
};

#endif // ARDUINO_ARCH_ESP32
