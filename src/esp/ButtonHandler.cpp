#ifdef ARDUINO_ARCH_ESP32

#include "esp/ButtonHandler.h"

#include "esp/DeskService.h"
#include "esp/StatusHandler.h"
#include "esp/secrets.h"

void ButtonHandler::begin()
{
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

bool ButtonHandler::getDown() const { return (states & (0b1U << 1U)) != 0U; }

bool ButtonHandler::getDownSimulation() const { return simulateDown.first; }

bool ButtonHandler::getState3() const { return (states & (0b1U << 2U)) != 0U; }

bool ButtonHandler::getState4() const { return (states & (0b1U << 3U)) != 0U; }

bool ButtonHandler::getUp() const { return (states & 0b1U) != 0U; }

bool ButtonHandler::getUpSimulation() const { return simulateUp.first; }

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
 * @brief Updates the down-drive state from its input pin.
 *
 * Records the physical down-drive state, updates the status indicator for an
 * active down-drive request, and marks the device state for publication.
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
 * @brief Updates the upward drive state after a hardware interrupt.
 *
 * Records the active state of the upward drive input, updates the status indicator
 * when upward driving is requested, and marks the device state for publication.
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

void ButtonHandler::setStatus()
{
    !getDownSimulation() && !getUpSimulation() && ((getDown() && !getUp()) || (getUp() && !getDown()))
        ? StatusHandler::setGreen()
        : StatusHandler::setBlue();
}

#endif // ARDUINO_ARCH_ESP32
