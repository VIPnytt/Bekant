#pragma once

#ifdef ARDUINO_ARCH_ESP32

#include <ArduinoJson.h>
#include <atomic>
#include <esp_timer.h>

class ToneHandler
{
private:
    esp_timer_handle_t timer{nullptr};

    std::atomic<bool> tick{false};

    uint16_t duration{0b1U << 9U};
    uint16_t frequency{0b1U << 12U};

    void scheduleSave();

    static void onTimer(void *arg);

public:
    void begin();

    void handle();

    void parse(const JsonObjectConst &doc);

    [[nodiscard]] uint16_t getDuration() const;

    [[nodiscard]] uint16_t getFrequency() const;
};

#endif // ARDUINO_ARCH_ESP32
