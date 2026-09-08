#pragma once

#ifdef ARDUINO_ARCH_ESP32

#include "esp/secrets.h"

#include <NeoPixelBus.h>

class StatusHandler
{
private:
    bool pending{true};

    unsigned long lastMillis{0U};

#ifdef PIN_LED
    NeoPixelBus<NeoGrbFeature, NeoWs2812Method> led{1U, PIN_LED};
#endif // PIN_LED

    RgbColor color{0xFFU, 0xFFU, 0xFFU};

    void fade();

public:
    /**
     * Processes pending status LED updates.
     */
    void handle();

    /**
     * Selects blue for the status LED.
     */
    void setBlue();

    /**
     * Selects green for the status LED.
     */
    void setGreen();

    /**
     * Disables the status LED.
     *
     * @param force Forces the status update when true.
     */
    void setNone(bool force = false);

    /**
     * Selects red for the status LED.
     */
    void setRed();

    /**
     * Selects white for the status LED.
     *
     * @param force Forces the status update when true.
     */
    void setWhite(bool force = false);
};

#endif // ARDUINO_ARCH_ESP32
