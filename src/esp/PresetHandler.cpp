#ifdef ARDUINO_ARCH_ESP32

#include "esp/PresetHandler.h"

#include "esp/DeskService.h"
#include "esp/constants.h"

#include <nvs.h>

/**
 * @brief Restores stored high and low desk-height presets when available.
 */
void PresetHandler::begin()
{
    nvs_handle_t handle{};
    if (nvs_open("preset", nvs_open_mode_t::NVS_READONLY, &handle) == ESP_OK)
    {
        nvs_get_u16(handle, "high", &high);
        nvs_get_u16(handle, "low", &low);
        nvs_close(handle);
    }
}

/**
 * @brief Persists changed presets after the deferred save interval.
 *
 * Failed storage operations leave the presets pending for a later retry.
 */
void PresetHandler::handle()
{
    if (!saved && millis() - lastMillis > 0b1U << 16U)
    {
        nvs_handle_t handle{};
        if (nvs_open("preset", nvs_open_mode_t::NVS_READWRITE, &handle) == ESP_OK)
        {
            saved = nvs_set_u16(handle, "high", high) == ESP_OK && nvs_set_u16(handle, "low", low) == ESP_OK &&
                    nvs_commit(handle) == ESP_OK;
            nvs_close(handle);
        }
        lastMillis = millis();
    }
}

/**
 * @brief Converts the stored high preset encoder value to a desk height.
 *
 * @return Converted high preset height in centimeters.
 */
float PresetHandler::getHigh() const { return DeskService::decode(high); }

/**
 * @brief Converts the stored low preset encoder value to a desk height.
 *
 * @return Converted low preset height in centimeters.
 */
float PresetHandler::getLow() const { return DeskService::decode(low); }

/**
 * @brief Requests movement to the high desk-height preset stored by the AVR.
 */
void PresetHandler::setHigh() { ConsoleHandler::send(ConsoleHandler::Command::PRESET_HIGH); }

/**
 * @brief Requests that the AVR store a high desk-height preset.
 *
 * Heights outside the configured reference range are ignored.
 *
 * @param height Preset height in centimeters.
 */
void PresetHandler::setHigh(float height)
{
    if (height <= ReferenceHeight::heightHigh && height >= ReferenceHeight::heightLow)
    {
        ConsoleHandler::send(ConsoleHandler::Command::PRESET_HIGH, DeskService::encode(height));
    }
}

/**
 * @brief Reconciles the high desk-height preset reported by the AVR.
 *
 * When the AVR reports the empty-EEPROM value `0xFFFF` and the ESP32's stored
 * preset is within the reference encoder range, sends the stored value back to
 * the AVR. Otherwise, stores a changed reported value and marks the device
 * state for persistence and publication.
 *
 * @param encoder Preset encoder value reported by the AVR; `0xFFFF` denotes empty EEPROM.
 */
void PresetHandler::setHigh(uint16_t encoder)
{
    if (encoder == 0xFFFFU && high <= ReferenceHeight::encoderHigh && high >= ReferenceHeight::encoderLow)
    {
        ConsoleHandler::send(ConsoleHandler::Command::PRESET_HIGH, high);
    }
    else if (encoder != high)
    {
        high = encoder;
        lastMillis = millis();
        saved = false;
        desk.setPending();
    }
}

/**
 * @brief Requests movement to the low desk-height preset stored by the AVR.
 */
void PresetHandler::setLow() { ConsoleHandler::send(ConsoleHandler::Command::PRESET_LOW); }

/**
 * @brief Requests that the AVR store a low desk-height preset.
 *
 * Heights outside the configured reference range are ignored.
 *
 * @param height Preset height in centimeters.
 */
void PresetHandler::setLow(float height)
{
    if (height <= ReferenceHeight::heightHigh && height >= ReferenceHeight::heightLow)
    {
        ConsoleHandler::send(ConsoleHandler::Command::PRESET_LOW, DeskService::encode(height));
    }
}

/**
 * @brief Reconciles the low desk-height preset reported by the AVR.
 *
 * When the AVR reports the empty-EEPROM value `0xFFFF` and the ESP32's stored
 * preset is within the reference encoder range, sends the stored value back to
 * the AVR. Otherwise, stores a changed reported value and marks the device
 * state for persistence and publication.
 *
 * @param encoder Preset encoder value reported by the AVR; `0xFFFF` denotes empty EEPROM.
 */
void PresetHandler::setLow(uint16_t encoder)
{
    if (encoder == 0xFFFFU && low <= ReferenceHeight::encoderHigh && low >= ReferenceHeight::encoderLow)
    {
        ConsoleHandler::send(ConsoleHandler::Command::PRESET_LOW, low);
    }
    else if (encoder != low)
    {
        low = encoder;
        lastMillis = millis();
        saved = false;
        desk.setPending();
    }
}

#endif // ARDUINO_ARCH_ESP32
