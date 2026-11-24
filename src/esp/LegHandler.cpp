#ifdef ARDUINO_ARCH_ESP32

#include "esp/LegHandler.h"

#include "esp/DeskService.h"

#include <nvs.h>
#include <utility>

/**
 * @brief Initializes the deferred save timer and restores stored encoder positions for nodes 8 and 9 when available.
 */
void LegHandler::begin()
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
    if (nvs_open("leg", nvs_open_mode_t::NVS_READONLY, &handle) == ESP_OK)
    {
        nvs_get_u16(handle, "encoder8", &encoder8);
        nvs_get_u16(handle, "encoder9", &encoder9);
        nvs_close(handle);
    }
}

/**
 * @brief Persists changed leg encoder positions after the deferred save interval.
 *
 * Failed storage operations re-arm the timer for a later retry.
 */
void LegHandler::handle()
{
    if (tick.exchange(false, std::memory_order_acq_rel))
    {
        bool saved{false};
        nvs_handle_t handle{};
        if (nvs_open("leg", nvs_open_mode_t::NVS_READWRITE, &handle) == ESP_OK)
        {
            saved = nvs_set_u16(handle, "encoder8", encoder8) == ESP_OK &&
                    nvs_set_u16(handle, "encoder9", encoder9) == ESP_OK && nvs_commit(handle) == ESP_OK;
            nvs_close(handle);
        }
        if (!saved)
        {
            scheduleSave();
        }
    }
}

/**
 * @brief Returns the latest raw leg encoder positions.
 *
 * @return Node 8's encoder position followed by node 9's encoder position.
 */
std::pair<uint16_t, uint16_t> LegHandler::getEncoders() const { return {encoder8, encoder9}; }

/**
 * @brief Reports whether both leg nodes are idle.
 *
 * @return `true` when both latest node states indicate an idle condition.
 */
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
 * @brief Marks pending encoder positions as due for persistence when the debounce timer expires.
 *
 * @param arg Pointer to the owning LegHandler instance.
 */
void LegHandler::onTimer(void *arg) { static_cast<LegHandler *>(arg)->tick.store(true, std::memory_order_release); }

/**
 * @brief Arms or restarts the deferred NVS save timer.
 */
void LegHandler::scheduleSave()
{
    if (timer != nullptr)
    {
        esp_timer_stop(timer);
        tick.store(false, std::memory_order_release);
        esp_timer_start_once(timer, 0b1ULL << 26U);
    }
}

/**
 * @brief Updates node 8's encoder position and state.
 *
 * Changes update the status indicator and are marked for publication. Position changes are also scheduled for
 * persistence.
 *
 * @param position Encoder position reported by the node.
 * @param state State reported by the node.
 */
void LegHandler::setNode8(uint16_t position, uint8_t state)
{
    const bool positionChanged{position != encoder8};
    const bool stateChanged{state != state8};
    if (positionChanged)
    {
        encoder8 = position;
        scheduleSave();
    }
    if (stateChanged)
    {
        state8 = state;
    }
    if (positionChanged || stateChanged)
    {
        desk.setPending();
        desk.setStatus();
    }
}

/**
 * @brief Updates node 9's encoder position and state.
 *
 * Changes update the status indicator and are marked for publication. Position changes are also scheduled for
 * persistence.
 *
 * @param position Encoder position reported by the node.
 * @param state State reported by the node.
 */
void LegHandler::setNode9(uint16_t position, uint8_t state)
{
    const bool positionChanged{position != encoder9};
    const bool stateChanged{state != state9};
    if (positionChanged)
    {
        encoder9 = position;
        scheduleSave();
    }
    if (stateChanged)
    {
        state9 = state;
    }
    if (positionChanged || stateChanged)
    {
        desk.setPending();
        desk.setStatus();
    }
}

#endif // ARDUINO_ARCH_ESP32
