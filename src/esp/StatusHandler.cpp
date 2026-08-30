#ifdef ARDUINO_ARCH_ESP32

#include "esp/StatusHandler.h"

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

void StatusHandler::setBlue()
{
    color.B = 0xFFU;
    color.G = 0U;
    color.R = 0U;
    pending = true;
}

void StatusHandler::setGreen()
{
    color.B = 0U;
    color.G = 0xFFU;
    color.R = 0U;
    pending = true;
}

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

void StatusHandler::setRed()
{
    color.B = 0U;
    color.G = 0U;
    color.R = 0xFFU;
    pending = true;
}

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
