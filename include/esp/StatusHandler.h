#pragma once

#ifdef ARDUINO_ARCH_ESP32

#include <atomic>
#include <cstdint>
#include <esp_timer.h>

class StatusHandler
{
private:
    struct Color
    {
        uint8_t r{0U};
        uint8_t g{0U};
        uint8_t b{0U};

        constexpr bool operator==(const Color &) const = default;
    };

    static constexpr Color black{0U, 0U, 0U};
    static constexpr Color blue{0U, 0U, 0xFFU};
    static constexpr Color green{0U, 0xFFU, 0U};
    static constexpr Color red{0xFFU, 0U, 0U};
    static constexpr Color white{0xFFU, 0xFFU, 0xFFU};

    static inline bool pending{true};

    esp_timer_handle_t timer{nullptr};

    std::atomic<bool> tick{false};

    static inline Color color{black};

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
