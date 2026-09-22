#ifdef ARDUINO_ARCH_ESP32

#include "esp/ButtonHandler.h"

#include "esp/DeskService.h"
#include "esp/StatusHandler.h"
#include "esp/secrets.h"

/**
 * @brief Initializes the optional down- and up-button simulation outputs.
 *
 * Releases each configured open-drain output and attaches its change interrupt.
 */
void ButtonHandler::begin()
{
#if defined(PIN_TPDN) && defined(PIN_TPUP) && SOC_GPIO_SUPPORT_DEEPSLEEP_WAKEUP
    esp_deep_sleep_enable_gpio_wakeup((1ULL << static_cast<unsigned int>(PIN_TPDN)) |
                                          (1ULL << static_cast<unsigned int>(PIN_TPUP)),
                                      esp_deepsleep_gpio_wake_up_mode_t::ESP_GPIO_WAKEUP_GPIO_LOW);
#elif defined(PIN_TPDN) && defined(PIN_TPUP) && SOC_PM_SUPPORT_EXT_WAKEUP && CONFIG_IDF_TARGET_ESP32
    esp_sleep_enable_ext0_wakeup(static_cast<gpio_num_t>(PIN_TPDN), LOW);
    esp_sleep_enable_ext1_wakeup(1ULL << static_cast<unsigned int>(PIN_TPUP),
                                 esp_sleep_ext1_wakeup_mode_t::ESP_EXT1_WAKEUP_ALL_LOW);
#endif // defined(PIN_TPDN) && defined(PIN_TPUP) && SOC_GPIO_SUPPORT_DEEPSLEEP_WAKEUP
#ifdef PIN_TPDN
    pinMode(PIN_TPDN, OUTPUT_OPEN_DRAIN);
#endif // PIN_TPDN
#ifdef PIN_TPUP
    pinMode(PIN_TPUP, OUTPUT_OPEN_DRAIN);
#endif // PIN_TPUP
#ifdef PIN_TPDN
    digitalWrite(PIN_TPDN, HIGH);
#endif // PIN_TPDN
#ifdef PIN_TPUP
    digitalWrite(PIN_TPUP, HIGH);
#endif // PIN_TPUP
#ifdef PIN_TPDN
    attachInterrupt(PIN_TPDN, onDown, CHANGE);
#endif // PIN_TPDN
#ifdef PIN_TPUP
    attachInterrupt(PIN_TPUP, onUp, CHANGE);
#endif // PIN_TPUP
}

/**
 * @brief Reports whether the AVR's latest button state has the down button pressed.
 *
 * @return `true` when the down-button bit is set.
 */
bool ButtonHandler::getDown() const { return (states & (0b1U << 1U)) != 0U; }

/**
 * @brief Reports whether down-button simulation is requested.
 *
 * @return `true` while the down-button simulation output is requested active.
 */
bool ButtonHandler::getDownSimulation() { return simulateDown.first; }

/**
 * @brief Reports whether the AVR's latest button state has button 3 pressed.
 *
 * @return `true` when the button-3 bit is set.
 */
bool ButtonHandler::getState3() const { return (states & (0b1U << 2U)) != 0U; }

/**
 * @brief Reports whether the AVR's latest button state has button 4 pressed.
 *
 * @return `true` when the button-4 bit is set.
 */
bool ButtonHandler::getState4() const { return (states & (0b1U << 3U)) != 0U; }

/**
 * @brief Reports whether the AVR's latest button state has the up button pressed.
 *
 * @return `true` when the up-button bit is set.
 */
bool ButtonHandler::getUp() const { return (states & 0b1U) != 0U; }

/**
 * @brief Reports whether up-button simulation is requested.
 *
 * @return `true` while the up-button simulation output is requested active.
 */
bool ButtonHandler::getUpSimulation() { return simulateUp.first; }

/**
 * @brief Releases requested button-simulation outputs and clears their requested states.
 */
void ButtonHandler::resetSimulation()
{
#ifdef PIN_TPDN
    if (simulateDown.first)
    {
        digitalWrite(PIN_TPDN, HIGH);
        simulateDown.first = false;
    }
#endif // PIN_TPDN
#ifdef PIN_TPUP
    if (simulateUp.first)
    {
        digitalWrite(PIN_TPUP, HIGH);
        simulateUp.first = false;
    }
#endif // PIN_TPUP
}

/**
 * @brief Handles a change on the optional down-button simulation line.
 *
 * Records whether the line is asserted, updates the status indicator for an
 * active simulation request, and marks the device state for publication.
 */
void ButtonHandler::onDown()
{
#ifdef PIN_TPDN
    ButtonHandler::simulateDown.second = digitalRead(PIN_TPDN) == LOW;
    if (ButtonHandler::simulateDown.first)
    {
        ButtonHandler::simulateDown.second ? StatusHandler::setWhite(true) : StatusHandler::setRed();
    }
    desk.setPending();
#endif // PIN_TPDN
}

/**
 * @brief Handles a change on the optional up-button simulation line.
 *
 * Records whether the line is asserted, updates the status indicator for an
 * active simulation request, and marks the device state for publication.
 */
void ButtonHandler::onUp()
{
#ifdef PIN_TPUP
    ButtonHandler::simulateUp.second = digitalRead(PIN_TPUP) == LOW;
    if (ButtonHandler::simulateUp.first)
    {
        ButtonHandler::simulateUp.second ? StatusHandler::setWhite(true) : StatusHandler::setRed();
    }
    desk.setPending();
#endif // PIN_TPUP
}

/**
 * @brief Controls the optional output that simulates pressing the desk's down button.
 *
 * Marks the status as an error when activation is requested but the observed
 * output state is not active. Has no effect when down-button simulation is not configured.
 *
 * @param state Whether to activate the simulated down-button press.
 */
void ButtonHandler::setSimulateDown(bool state)
{
#ifdef PIN_TPDN
    simulateDown.first = state;
    if (simulateDown.first && simulateDown.first != simulateDown.second)
    {
        StatusHandler::setRed();
    }
    digitalWrite(PIN_TPDN, state ? LOW : HIGH);
#endif // PIN_TPDN
}

/**
 * @brief Controls the optional output that simulates pressing the desk's up button.
 *
 * Marks the status as an error when activation is requested but the observed
 * output state is not active. Has no effect when up-button simulation is not configured.
 *
 * @param state Whether to activate the simulated up-button press.
 */
void ButtonHandler::setSimulateUp(bool state)
{
#ifdef PIN_TPUP
    simulateUp.first = state;
    if (simulateUp.first && simulateUp.first != simulateUp.second)
    {
        StatusHandler::setRed();
    }
    digitalWrite(PIN_TPUP, simulateUp.first ? LOW : HIGH);
#endif // PIN_TPUP
}

/**
 * @brief Updates the physical button states and requests state publication when they change.
 *
 * @param flags Button-state bitmask with up, down, button 3, and button 4 in bits 0 through 3, respectively.
 */
void ButtonHandler::setStates(uint8_t flags)
{
    if (flags != states)
    {
        states = flags;
        StatusHandler::setWhite();
        desk.setPending();
    }
}

/**
 * @brief Selects the status color from physical and simulated directional-button activity.
 *
 * Uses green when exactly one physical direction button is pressed without a
 * simulation request, and blue otherwise.
 */
void ButtonHandler::setStatus()
{
    !getDownSimulation() && !getUpSimulation() && ((getDown() && !getUp()) || (getUp() && !getDown()))
        ? StatusHandler::setGreen()
        : StatusHandler::setBlue();
}

#endif // ARDUINO_ARCH_ESP32
