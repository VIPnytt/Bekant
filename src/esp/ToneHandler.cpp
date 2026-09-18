#ifdef ARDUINO_ARCH_ESP32

#include "esp/ToneHandler.h"

#include "esp/DeskService.h"

#include <nvs.h>

/**
 * @brief Restores stored tone duration and frequency settings when available.
 */
void ToneHandler::begin()
{
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
 * Failed storage operations leave the settings pending for a later retry.
 */
void ToneHandler::handle()
{
    if (!saved && millis() - lastMillis > 0b1U << 16U)
    {
        nvs_handle_t handle{};
        if (nvs_open("tone", nvs_open_mode_t::NVS_READWRITE, &handle) == ESP_OK)
        {
            saved = nvs_set_u16(handle, "duration", duration) == ESP_OK &&
                    nvs_set_u16(handle, "frequency", frequency) == ESP_OK && nvs_commit(handle) == ESP_OK;
            nvs_close(handle);
        }
        lastMillis = millis();
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
    if (doc["duration"].is<uint16_t>())
    {
        const uint16_t _duration{doc["duration"].as<uint16_t>()};
        if (_duration != duration && _duration != 0U)
        {
            duration = _duration;
            lastMillis = millis();
            saved = false;
            desk.setPending();
        }
    }
    if (doc["frequency"].is<uint16_t>())
    {
        const uint16_t _frequency{doc["frequency"].as<uint16_t>()};
        if (_frequency != frequency && _frequency != 0U)
        {
            frequency = _frequency;
            lastMillis = millis();
            saved = false;
            desk.setPending();
        }
    }
    ConsoleHandler::send(ConsoleHandler::Command::TONE, frequency, duration);
}

#endif // ARDUINO_ARCH_ESP32
