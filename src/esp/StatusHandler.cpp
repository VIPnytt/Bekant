#ifdef ARDUINO_ARCH_ESP32

#include "esp/StatusHandler.h"

#include "esp/secrets.h"

#include <FastLED.h>

/**
 * @brief Registers the configured status LED with FastLED.
 */
void StatusHandler::begin()
{
#ifdef PIN_LED
    FastLED.addLeds<WS2812, PIN_LED, fl::EOrder::GRB>(&color, 1);
#endif // PIN_LED
}

/**
 * @brief Updates the status LED and advances its color state periodically.
 *
 * Applies pending color changes immediately and gradually fades the current
 * color after approximately 256 milliseconds without a pending update.
 */
void StatusHandler::handle()
{
    if (pending)
    {
#ifdef PIN_LED
        FastLED.show();
#endif // PIN_LED
        lastMillis = millis();
        pending = false;
    }
    else if (millis() - lastMillis > (0b1U << 8U))
    {
        lastMillis = millis();
        if (color != CRGB::Black)
        {
            --color;
            pending = true;
        }
    }
}

/**
 * @brief Sets the status color to full-intensity blue.
 */
void StatusHandler::setBlue()
{
    color = CRGB::Blue;
    pending = true;
}

/**
 * @brief Sets the status color to full-intensity green.
 */
void StatusHandler::setGreen()
{
    color = CRGB::Green;
    pending = true;
}

/**
 * @brief Clears the status indicator color when clearing is applicable.
 *
 * @param force Forces the color to be cleared regardless of its current value.
 */
void StatusHandler::setNone(bool force)
{
    if (color[0U] == 0U || (color[0U] == color[1U] && color[1U] == color[2U]) || force)
    {
        color = CRGB::Black;
        pending = true;
    }
}

/**
 * @brief Sets the status color to full-intensity red.
 */
void StatusHandler::setRed()
{
    color = CRGB::Red;
    pending = true;
}

/**
 * @brief Sets the status color to full-intensity white when permitted.
 *
 * @param force Whether to set white regardless of the current color.
 */
void StatusHandler::setWhite(bool force)
{
    if ((color[0U] == color[1U] && color[1U] == color[2U]) || force)
    {
        color = CRGB::White;
        pending = true;
    }
}

#endif // ARDUINO_ARCH_ESP32
