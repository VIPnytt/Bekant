#pragma once

#ifdef ARDUINO_ARCH_ESP32

#include <atomic>
#include <cstdint>
#include <esp_timer.h>

class PresetHandler
{
private:
    esp_timer_handle_t timer{nullptr};

    std::atomic<bool> tick{false};

    uint16_t high{0U};
    uint16_t low{0U};

    void scheduleSave();

    static void onTimer(void *arg);

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
