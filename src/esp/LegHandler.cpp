#ifdef ARDUINO_ARCH_ESP32

#include "esp/LegHandler.h"

#include "esp/ButtonHandler.h"
#include "esp/DeskService.h"
#include "esp/IssueHandler.h"

#include <nvs.h>

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

std::pair<uint16_t, uint16_t> LegHandler::getEncoders() const { return {encoder8, encoder9}; }

std::pair<float, float> LegHandler::getLegs() const
{
    return {DeskService::decode(static_cast<float>(encoder8)), DeskService::decode(static_cast<float>(encoder9))};
}

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
        IssueHandler::setNode8(0U);
        saved = false;
        desk.setPending();
        setStatus();
    }
    else if (position != encoder8)
    {
        encoder8 = position;
        IssueHandler::setNode8(0U);
        saved = false;
        desk.setPending();
        setStatus();
    }
    else if (state != state8)
    {
        state8 = state;
        IssueHandler::setNode8(0U);
        desk.setPending();
        setStatus();
    }
    else
    {
        IssueHandler::setNode8(0U);
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
        IssueHandler::setNode9(0U);
        saved = false;
        desk.setPending();
        setStatus();
    }
    else if (position != encoder9)
    {
        encoder9 = position;
        IssueHandler::setNode9(0U);
        saved = false;
        desk.setPending();
        setStatus();
    }
    else if (state != state9)
    {
        state9 = state;
        IssueHandler::setNode9(0U);
        desk.setPending();
        setStatus();
    }
    else
    {
        IssueHandler::setNode9(0U);
    }
}

/**
 * @brief Selects the status indicator color from motor, button, and drive activity.
 *
 * @details Uses white for idle motor states, green for exclusive manual button activity
 * without drive output activity, and blue for all other states.
 */
void LegHandler::setStatus()
{
    (state8 == 0U || state8 == 0x25U || state8 == 0x60U) && (state9 == 0U || state9 == 0x25U || state9 == 0x60U)
        ? StatusHandler::setWhite(true)
        : ButtonHandler::setStatus();
}

#endif // ARDUINO_ARCH_ESP32
