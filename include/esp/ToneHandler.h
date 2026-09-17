#pragma once

#ifdef ARDUINO_ARCH_ESP32

#include <ArduinoJson.h> // NOLINT(misc-include-cleaner)

class ToneHandler
{
private:
    static inline bool saved{true};

    static inline uint16_t duration{0b1U << 9U};
    static inline uint16_t frequency{0b1U << 12U};

    static inline unsigned long lastMillis{0UL};

public:
    void begin();

    void handle();

    uint16_t getDuration() const;

    uint16_t getFrequency() const;

    static void parse(const JsonObjectConst &doc);
};

#endif // ARDUINO_ARCH_ESP32
