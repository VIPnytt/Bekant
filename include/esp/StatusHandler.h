#pragma once

#ifdef ARDUINO_ARCH_ESP32

#include <crgb.h>

class StatusHandler
{
private:
    unsigned long lastMillis{0U};

    static inline bool pending{true};

    static inline CRGB color{CRGB::Black};

    void fade();

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
