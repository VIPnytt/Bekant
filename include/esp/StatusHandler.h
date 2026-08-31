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

    /**
 * Updates the status indicator.
 */
void handle();

/**
 * Selects blue for the status indicator.
 */
void setBlue();

/**
 * Selects green for the status indicator.
 */
void setGreen();

/**
 * Clears the status indicator.
 *
 * @param force Whether to apply the update even when no status change is pending.
 */
void setNone(bool force = false);

/**
 * Selects red for the status indicator.
 */
void setRed();

/**
 * Selects white for the status indicator.
 *
 * @param force Whether to apply the update even when no status change is pending.
 */
void setWhite(bool force = false);
RgbColor color{0xFFU, 0xFFU, 0xFFU};

    void fade();

public:
    void handle();

    void setBlue();
    void setGreen();
    void setNone(bool force = false);
    void setRed();
    void setWhite(bool force = false);
};

#endif // ARDUINO_ARCH_ESP32
