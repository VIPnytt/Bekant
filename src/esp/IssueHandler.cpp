#ifdef ARDUINO_ARCH_ESP32

#include "esp/IssueHandler.h"

#include "esp/DeskService.h"
#include "esp/secrets.h"

/**
 * @brief Appends descriptions of a firmware mismatch, reset causes, and recorded initialization or communication
 * errors.
 *
 * @param list JSON array to append to.
 */
void IssueHandler::getIssues(JsonArray &list)
{
    getComs(list);
    getLegs(list);
    getResets(list);
    if (version != fingerprint(DeskService::version))
    {
        list.add("AVR: version mismatch");
    }
}

void IssueHandler::getComs(JsonArray &list)
{
    if ((legsRx & 0b1U) != 0U)
    {
        list.add("USART0: parity error");
    }
    if ((legsRx & (0b1U << 1U)) != 0U)
    {
        list.add("USART0: data overrun");
    }
    if ((legsRx & (0b1U << 2U)) != 0U)
    {
        list.add("USART0: frame error");
    }
    if ((consoleTx & 0b1U) != 0U)
    {
        list.add("USART1: parity error");
    }
    if ((consoleTx & (0b1U << 1U)) != 0U)
    {
        list.add("USART1: data overrun");
    }
    if ((consoleTx & (0b1U << 2U)) != 0U)
    {
        list.add("USART1: frame error");
    }
    switch (consoleRx)
    {
    case hardwareSerial_error_t::UART_BREAK_ERROR:
        list.add("UART: break");
        break;
    case hardwareSerial_error_t::UART_BUFFER_FULL_ERROR:
        list.add("UART: buffer full");
        break;
    case hardwareSerial_error_t::UART_FIFO_OVF_ERROR:
        list.add("UART: FIFO overflow");
        break;
    case hardwareSerial_error_t::UART_FRAME_ERROR:
        list.add("UART: frame error");
        break;
    case hardwareSerial_error_t::UART_PARITY_ERROR:
        list.add("UART: parity error");
        break;
    default:
        break;
    }
}

void IssueHandler::getLegs(JsonArray &list)
{
    if ((node8 & 0b1U) != 0U)
    {
        list.add("node 8: no response");
    }
    if ((node8 & (0b1U << 1U)) != 0U)
    {
        list.add("node 8: checksum mismatch");
    }
    if ((node9 & 0b1U) != 0U)
    {
        list.add("node 9: no response");
    }
    if ((node9 & (0b1U << 1U)) != 0U)
    {
        list.add("node 9: checksum mismatch");
    }
    if ((initialization & 0b1U) != 0U)
    {
        list.add("probe A: no response");
    }
    if ((initialization & (0b1U << 1U)) != 0U)
    {
        list.add("probe A: checksum mismatch");
    }
    if ((initialization & (0b1U << 2U)) != 0U)
    {
        list.add("probe B: no response");
    }
    if ((initialization & (0b1U << 3U)) != 0U)
    {
        list.add("probe B: checksum mismatch");
    }
}

bool IssueHandler::getNode8() { return node8 == 0U; }

bool IssueHandler::getNode9() { return node9 == 0U; }

void IssueHandler::getResets(JsonArray &list)
{
    if ((resetReason & (0b1U << 2U)) != 0U)
    {
        list.add("MCUSR: brown-out reset");
    }
    if ((resetReason & (0b1U << 3U)) != 0U)
    {
        list.add("MCUSR: watchdog reset");
    }
    switch (esp_reset_reason())
    {
    case esp_reset_reason_t::ESP_RST_PANIC:
        list.add("ESP32: panic reset");
        break;
    case esp_reset_reason_t::ESP_RST_INT_WDT:
        list.add("ESP32: interrupt watchdog reset");
        break;
    case esp_reset_reason_t::ESP_RST_TASK_WDT:
        list.add("ESP32: task watchdog reset");
        break;
    case esp_reset_reason_t::ESP_RST_WDT:
        list.add("ESP32: watchdog reset");
        break;
    case esp_reset_reason_t::ESP_RST_BROWNOUT:
        list.add("ESP32: brown-out reset");
        break;
    case esp_reset_reason_t::ESP_RST_PWR_GLITCH:
        list.add("ESP32: power glitch reset");
        break;
    case esp_reset_reason_t::ESP_RST_CPU_LOCKUP:
        list.add("ESP32: CPU lock-up reset");
        break;
    default:
        break;
    }
}

/**
 * @brief Records the latest hardware serial receive error and signals an error state.
 *
 * @param error Hardware serial error to store.
 */
void IssueHandler::onReceiveError(hardwareSerial_error_t error)
{
    ESP_LOGW("hardwareSerial_error_t", "%u", static_cast<unsigned int>(error));
    if (error != consoleRx)
    {
        consoleRx = error;
        desk.setPending();
    }
    if (consoleRx != hardwareSerial_error_t::UART_NO_ERROR)
    {
        StatusHandler::setRed();
    }
}

void IssueHandler::clear()
{
    node8 = 0U;
    node9 = 0U;
    initialization = 0U;
    legsRx = 0U;
    consoleRx = hardwareSerial_error_t::UART_NO_ERROR;
    consoleTx = 0U;
    resetReason = 0U;
}

/**
 * @brief Records console USART error flags and signals an error state.
 *
 * @param flags AVR USART status flags.
 */
void IssueHandler::setConsoleTx(uint8_t flags)
{
    if (flags != consoleTx)
    {
        consoleTx = flags;
        desk.setPending();
    }
    StatusHandler::setRed();
}

/**
 * @brief Records leg initialization errors and signals an error state.
 *
 * @param flags Error bitmask with response and checksum failures in bits 0 and 1 for probe A and bits 2 and 3 for
 * probe B.
 */
void IssueHandler::setInitialization(uint8_t flags)
{
    if (flags != initialization)
    {
        initialization = flags;
        desk.setPending();
    }
    StatusHandler::setRed();
}

/**
 * @brief Records LIN USART error flags and signals an error state.
 *
 * @param flags AVR USART status flags.
 */
void IssueHandler::setLegsRx(uint8_t flags)
{
    if (flags != legsRx)
    {
        legsRx = flags;
        desk.setPending();
    }
    StatusHandler::setRed();
}

/**
 * @brief Records node 8 communication errors and signals an error state.
 *
 * @param flags Error bitmask with bit 0 for no response and bit 1 for a checksum mismatch.
 */
void IssueHandler::setNode8(uint8_t flags)
{
    if (flags != node8)
    {
        node8 = flags;
        desk.setPending();
    }
    if (node8 != 0U)
    {
        StatusHandler::setRed();
    }
}

/**
 * @brief Records node 9 communication errors and signals an error state.
 *
 * @param flags Error bitmask with bit 0 for no response and bit 1 for a checksum mismatch.
 */
void IssueHandler::setNode9(uint8_t flags)
{
    if (flags != node9)
    {
        node9 = flags;
        desk.setPending();
    }
    if (node9 != 0U)
    {
        StatusHandler::setRed();
    }
}

/**
 * @brief Stores the AVR reset-cause flags and requests state publication when they change.
 *
 * @param flags AVR MCUSR reset-cause bitmask.
 */
void IssueHandler::setResetReason(uint8_t flags)
{
    if (flags != resetReason)
    {
        resetReason = flags;
        desk.setPending();
    }
}

/**
 * @brief Records the AVR firmware fingerprint and signals a version mismatch.
 *
 * Marks the device state for publication when the fingerprint changes and sets the status
 * indicator to red when the AVR and ESP32 firmware fingerprints differ.
 *
 * @param hash AVR firmware fingerprint.
 */
void IssueHandler::setVersion(uint8_t hash)
{
    if (hash != version)
    {
        version = hash;
        desk.setPending();
    }
    if (version != fingerprint(DeskService::version))
    {
        StatusHandler::setRed();
    }
}

#endif // ARDUINO_ARCH_ESP32
