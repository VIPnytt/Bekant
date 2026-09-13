#ifdef ARDUINO_ARCH_ESP32

#include "esp/StatusHandler.h"

#include "esp/secrets.h"

#include <FastLED.h>

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
 * color after approximately 512 milliseconds without a pending update.
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
        fade();
        lastMillis = millis();
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
    if (color.red == 0U || (color.blue == color.green && color.green == color.red) || force)
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
    if ((color.blue == color.green && color.green == color.red) || force)
    {
        color = CRGB::White;
        pending = true;
    }
}

/**
 * @brief Fades the current color by decreasing each nonzero RGB channel by one.
 */
void StatusHandler::fade()
{
    if (color.blue != 0U)
    {
        --color.blue;
        pending = true;
    }
    if (color.green != 0U)
    {
        --color.green;
        pending = true;
    }
    if (color.red != 0U)
    {
        --color.red;
        pending = true;
    }
}

#endif // ARDUINO_ARCH_ESP32
