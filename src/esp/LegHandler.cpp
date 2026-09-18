#ifdef ARDUINO_ARCH_ESP32

#include "esp/LegHandler.h"

#include "esp/DeskService.h"

#include <nvs.h>

/**
 * @brief Restores stored encoder positions for nodes 8 and 9 when available.
 */
void LegHandler::begin()
{
    nvs_handle_t handle{};
    if (nvs_open("leg", nvs_open_mode_t::NVS_READONLY, &handle) == ESP_OK)
    {
        nvs_get_u16(handle, "encoder8", &encoder8);
        nvs_get_u16(handle, "encoder9", &encoder9);
        nvs_close(handle);
    }
}

/**
 * @brief Periodically persists changed leg encoder positions.
 *
 * Failed storage operations leave the positions pending for a later retry.
 */
void LegHandler::handle()
{
    if (!saved && millis() - lastMillis > 0b1U << 16U)
    {
        nvs_handle_t handle{};
        if (nvs_open("leg", nvs_open_mode_t::NVS_READWRITE, &handle) == ESP_OK)
        {
            saved = nvs_set_u16(handle, "encoder8", encoder8) == ESP_OK &&
                    nvs_set_u16(handle, "encoder9", encoder9) == ESP_OK && nvs_commit(handle) == ESP_OK;
            nvs_close(handle);
        }
        lastMillis = millis();
    }
}

/**
 * @brief Returns the latest raw leg encoder positions.
 *
 * @return Node 8's encoder position followed by node 9's encoder position.
 */
std::pair<uint16_t, uint16_t> LegHandler::getEncoders() const { return {encoder8, encoder9}; }

bool LegHandler::getIdle() const
{
    return (state8 == 0U || state8 == 0x25U || state8 == 0x60U) && (state9 == 0U || state9 == 0x25U || state9 == 0x60U);
}

/**
 * @brief Converts the latest leg encoder positions to physical heights.
 *
 * @return Node 8's height followed by node 9's height, in centimeters.
 */
std::pair<float, float> LegHandler::getLegs() const
{
    return {DeskService::decode(static_cast<float>(encoder8)), DeskService::decode(static_cast<float>(encoder9))};
}

/**
 * @brief Returns the latest leg states.
 *
 * @return Node 8's state followed by node 9's state.
 */
std::pair<uint8_t, uint8_t> LegHandler::getStates() const { return {state8, state9}; }

/**
 * @brief Updates node 8 data and clears its communication error.
 *
 * Changes are marked for publication, and position changes are also marked for persistence.
 *
 * @param position Encoder position reported by the node.
 * @param state State reported by the node.
 */
void LegHandler::setNode8(uint16_t position, uint8_t state)
{
    if (position != encoder8 && state != state8)
    {
        encoder8 = position;
        state8 = state;
        saved = false;
        desk.setPending();
        desk.setStatus();
    }
    else if (position != encoder8)
    {
        encoder8 = position;
        saved = false;
        desk.setPending();
        desk.setStatus();
    }
    else if (state != state8)
    {
        state8 = state;
        desk.setPending();
        desk.setStatus();
    }
}

/**
 * @brief Updates node 9 data and clears its communication error.
 *
 * Changes are marked for publication, and position changes are also marked for persistence.
 *
 * @param position Encoder position reported by the node.
 * @param state State reported by the node.
 */
void LegHandler::setNode9(uint16_t position, uint8_t state)
{
    if (position != encoder9 && state != state9)
    {
        encoder9 = position;
        state9 = state;
        saved = false;
        desk.setPending();
        desk.setStatus();
    }
    else if (position != encoder9)
    {
        encoder9 = position;
        saved = false;
        desk.setPending();
        desk.setStatus();
    }
    else if (state != state9)
    {
        state9 = state;
        desk.setPending();
        desk.setStatus();
    }
}

#endif // ARDUINO_ARCH_ESP32
