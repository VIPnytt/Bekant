#ifdef ARDUINO_ARCH_ESP32

#include "esp/StatusHandler.h"

#include "esp/secrets.h"

#include <esp32-hal-rgb-led.h>

/**
 * @brief Configures the fade timer for the status LED.
 */
void StatusHandler::begin()
{
    const esp_timer_create_args_t args{
        .callback = onTimer,
        .arg = this,
        .dispatch_method = esp_timer_dispatch_t::ESP_TIMER_TASK,
        .skip_unhandled_events = true,
    };
    esp_timer_create(&args, &timer);
}

/**
 * @brief Updates the status LED and advances its color state periodically.
 *
 * Applies pending color changes immediately and gradually fades the current
 * color after approximately 256 milliseconds without a pending update.
 */
void StatusHandler::handle()
{
    if (!pending && tick.exchange(false, std::memory_order_acq_rel))
    {
        if (color != none)
        {
            if (color.r != 0U)
            {
                --color.r;
            }
            if (color.g != 0U)
            {
                --color.g;
            }
            if (color.b != 0U)
            {
                --color.b;
            }
            pending = true;
        }
    }
    if (pending)
    {
#ifdef PIN_LED
        rgbLedWriteOrdered(PIN_LED, rgb_led_color_order_t::LED_COLOR_ORDER_GRB, color.r, color.g, color.b);
#endif // PIN_LED
        pending = false;
        if (timer != nullptr)
        {
            esp_timer_stop(timer);
            tick.store(false, std::memory_order_release);
            if (color != none)
            {
                esp_timer_start_once(timer, 0b1ULL << 18U);
            }
        }
    }
}

void StatusHandler::onTimer(void *arg)
{
    static_cast<StatusHandler *>(arg)->tick.store(true, std::memory_order_release);
}

/**
 * @brief Sets the status color to full-intensity blue.
 */
void StatusHandler::setBlue()
{
    color = blue;
    pending = true;
}

/**
 * @brief Sets the status color to full-intensity green.
 */
void StatusHandler::setGreen()
{
    color = green;
    pending = true;
}

/**
 * @brief Clears the status indicator color when clearing is applicable.
 *
 * @param force Forces the color to be cleared regardless of its current value.
 */
void StatusHandler::setNone(bool force)
{
    if (color.r == 0U || (color.r == color.g && color.g == color.b) || force)
    {
        color = none;
        pending = true;
    }
}

/**
 * @brief Sets the status color to full-intensity red.
 */
void StatusHandler::setRed()
{
    color = red;
    pending = true;
}

/**
 * @brief Sets the status color to full-intensity white when permitted.
 *
 * @param force Whether to set white regardless of the current color.
 */
void StatusHandler::setWhite(bool force)
{
    if ((color.r == color.g && color.g == color.b) || force)
    {
        color = white;
        pending = true;
    }
}

#endif // ARDUINO_ARCH_ESP32
