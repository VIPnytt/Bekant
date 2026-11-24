#ifdef ARDUINO_ARCH_ESP32

#include "esp/ToneHandler.h"

#include "esp/DeskService.h"

#include <nvs.h>

/**
 * @brief Initializes the deferred save timer and restores stored tone settings when available.
 */
void ToneHandler::begin()
{
    if (timer == nullptr)
    {
        const esp_timer_create_args_t args{
            .callback{&onTimer},
            .arg{this},
            .dispatch_method{esp_timer_dispatch_t::ESP_TIMER_TASK},
            .skip_unhandled_events{false},
        };
        esp_timer_create(&args, &timer);
    }

    nvs_handle_t handle{};
    if (nvs_open("tone", nvs_open_mode_t::NVS_READONLY, &handle) == ESP_OK)
    {
        nvs_get_u16(handle, "duration", &duration);
        nvs_get_u16(handle, "frequency", &frequency);
        nvs_close(handle);
    }
}

/**
 * @brief Persists changed tone settings after the deferred save interval.
 *
 * Failed storage operations re-arm the timer for a later retry.
 */
void ToneHandler::handle()
{
    if (tick.exchange(false, std::memory_order_acq_rel))
    {
        bool saved{false};
        nvs_handle_t handle{};
        if (nvs_open("tone", nvs_open_mode_t::NVS_READWRITE, &handle) == ESP_OK)
        {
            saved = nvs_set_u16(handle, "duration", duration) == ESP_OK &&
                    nvs_set_u16(handle, "frequency", frequency) == ESP_OK && nvs_commit(handle) == ESP_OK;
            nvs_close(handle);
        }
        if (!saved)
        {
            scheduleSave();
        }
    }
}

/**
 * @brief Returns the configured tone duration.
 *
 * @return Duration in milliseconds.
 */
uint16_t ToneHandler::getDuration() const { return duration; }

/**
 * @brief Returns the configured tone frequency.
 *
 * @return Frequency in hertz.
 */
uint16_t ToneHandler::getFrequency() const { return frequency; }

/**
 * @brief Marks pending tone settings as due for persistence when the debounce timer expires.
 *
 * @param arg Pointer to the owning ToneHandler instance.
 */
void ToneHandler::onTimer(void *arg) { static_cast<ToneHandler *>(arg)->tick.store(true, std::memory_order_release); }

/**
 * @brief Applies tone settings from a JSON object and sends a playback command.
 *
 * Nonzero 16-bit duration and frequency values replace the current settings.
 * Missing, invalid, or zero values leave their respective settings unchanged.
 * Changed settings are marked for persistence and publication, and the command
 * always uses the resulting settings.
 *
 * @param doc Tone configuration object with duration in milliseconds and frequency in hertz.
 */
void ToneHandler::parse(const JsonObjectConst &doc)
{
    bool changed{false};
    if (doc["duration"].is<uint16_t>())
    {
        const uint16_t _duration{doc["duration"].as<uint16_t>()};
        if (_duration != duration && _duration != 0U)
        {
            duration = _duration;
            changed = true;
        }
    }
    if (doc["frequency"].is<uint16_t>())
    {
        const uint16_t _frequency{doc["frequency"].as<uint16_t>()};
        if (_frequency != frequency && _frequency != 0U)
        {
            frequency = _frequency;
            changed = true;
        }
    }
    if (changed)
    {
        scheduleSave();
        desk.setPending();
    }
    ConsoleHandler::send(ConsoleHandler::Command::TONE, frequency, duration);
}

/**
 * @brief Arms or restarts the deferred NVS save timer.
 */
void ToneHandler::scheduleSave()
{
    if (timer != nullptr)
    {
        esp_timer_stop(timer);
        tick.store(false, std::memory_order_release);
        esp_timer_start_once(timer, 0b1ULL << 26U);
    }
}

#endif // ARDUINO_ARCH_ESP32
