#ifdef ARDUINO_ARCH_ESP32

#include "esp/StatusHandler.h"

/**
 * @brief Updates the status LED and advances its color state periodically.
 *
 * Applies pending color changes immediately; otherwise, after approximately
 * 512 milliseconds, converts solid blue or green to white or fades the
 * current color.
 */
void StatusHandler::handle()
{
    if (pending)
    {
#ifdef PIN_LED
        led.SetPixelColor(0U, color);
        led.Show();
#endif // PIN_LED
        lastMillis = millis();
        pending = false;
    }
    else if (millis() - lastMillis > (0b1U << 9U))
    {
        color.R == 0U && (color.B == 0xFFU || color.G == 0xFFU) ? setWhite(true) : fade();
        lastMillis = millis();
    }
}

/**
 * @brief Sets the status color to full-intensity blue.
 */
void StatusHandler::setBlue()
{
    color.B = 0xFFU;
    color.G = 0U;
    color.R = 0U;
    pending = true;
}

/**
 * @brief Sets the status color to full-intensity green.
 */
void StatusHandler::setGreen()
{
    color.B = 0U;
    color.G = 0xFFU;
    color.R = 0U;
    pending = true;
}

/**
 * @brief Clears the status indicator color when clearing is applicable.
 *
 * @param force Forces the color to be cleared regardless of its current value.
 */
void StatusHandler::setNone(bool force)
{
    if (color.R == 0U || (color.B == color.G && color.G == color.R) || force)
    {
        color.B = 0U;
        color.G = 0U;
        color.R = 0U;
        pending = true;
    }
}

/**
 * @brief Sets the status color to full-intensity red.
 */
void StatusHandler::setRed()
{
    color.B = 0U;
    color.G = 0U;
    color.R = 0xFFU;
    pending = true;
}

/**
 * @brief Sets the status color to full-intensity white when permitted.
 *
 * @param force Whether to set white regardless of the current color.
 */
void StatusHandler::setWhite(bool force)
{
    if ((color.B == color.G && color.G == color.R) || force)
    {
        color.B = 0xFFU;
        color.G = 0xFFU;
        color.R = 0xFFU;
        pending = true;
    }
}

/**
 * @brief Fades the current color by decreasing each nonzero RGB channel by one.
 */
void StatusHandler::fade()
{
    if (color.B != 0U)
    {
        --color.B;
        pending = true;
    }
    if (color.G != 0U)
    {
        --color.G;
        pending = true;
    }
    if (color.R != 0U)
    {
        --color.R;
        pending = true;
    }
}

#endif // ARDUINO_ARCH_ESP32
