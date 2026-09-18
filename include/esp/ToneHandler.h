#pragma once

#ifdef ARDUINO_ARCH_ESP32

#include <ArduinoJson.h> // NOLINT(misc-include-cleaner)

class ToneHandler
{
private:
    bool saved{true};

    uint16_t duration{0b1U << 9U};
    uint16_t frequency{0b1U << 12U};

    unsigned long lastMillis{0UL};

public:
    void begin();

    void handle();

    void parse(const JsonObjectConst &doc);

    [[nodiscard]] uint16_t getDuration() const;

    [[nodiscard]] uint16_t getFrequency() const;
};

#endif // ARDUINO_ARCH_ESP32
