#pragma once

#ifdef ARDUINO_ARCH_ESP32

#include <atomic>
#include <crgb.h>
#include <esp_timer.h>

class StatusHandler
{
private:
    static inline bool pending{true};

    esp_timer_handle_t timer{nullptr};

    std::atomic<bool> tick{false};

    static inline CRGB color{CRGB::Black};

    static void onTimer(void *arg);

public:
    void begin();

    /**
     * Processes pending status LED updates.
     */
    void handle();

    /**
     * Selects blue for the status LED.
     */
    static void setBlue();

    /**
     * Selects green for the status LED.
     */
    static void setGreen();

    /**
     * Disables the status LED.
     *
     * @param force Forces the status update when true.
     */
    static void setNone(bool force = false);

    /**
     * Selects red for the status LED.
     */
    static void setRed();

    /**
     * Selects white for the status LED.
     *
     * @param force Forces the status update when true.
     */
    static void setWhite(bool force = false);
};

#endif // ARDUINO_ARCH_ESP32
