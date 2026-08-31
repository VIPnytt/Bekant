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
    void handle();

    void setBlue();
    void setGreen();
    void setNone(bool force = false);
    void setRed();
    void setWhite(bool force = false);
};

#endif // ARDUINO_ARCH_ESP32
