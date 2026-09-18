#ifdef ARDUINO_ARCH_ESP32

#include "esp/ConsoleHandler.h"

#include "esp/DeskService.h"
#include "esp/IssueHandler.h"
#include "esp/secrets.h"

#include <string_view>

/**
 * @brief Initializes the serial console and configures its communication pins.
 */
void ConsoleHandler::begin()
{
    pinMode(PIN_MISO, INPUT);
    pinMode(PIN_SCK, OUTPUT);
    Serial1.onReceiveError(&IssueHandler::onReceiveError);
    Serial1.begin(115'200UL, SerialConfig::SERIAL_8N1, PIN_MISO, PIN_SCK);
}

/**
 * @brief Processes a secondary-serial byte or forwards primary-serial input when none is available.
 */
void ConsoleHandler::handle()
{
    const int byte{Serial1.read()};
    if (byte != -1)
    {
        ESP_LOGV("RX", "0x%X", byte);
        if (lengthRx == 0U)
        {
            lengthRx = static_cast<size_t>(static_cast<uint8_t>(byte) >> 4U); // NOLINT(hicpp-signed-bitwise)
            stateRx = static_cast<State>(static_cast<uint8_t>(byte) & 0xFU);  // NOLINT(hicpp-signed-bitwise)
            bytesRx = 0U;
        }
        bufferRx.at(bytesRx++) = static_cast<uint8_t>(byte);
        if (bytesRx == lengthRx + 1U)
        {
            desk.parse(stateRx, std::span{bufferRx}.first(lengthRx + 1U));
            lengthRx = 0U;
        }
    }
    else
    {
        forward();
    }
}

/**
 * @brief Forwards a complete length-prefixed frame from the primary serial interface.
 *
 * Completed frames are transmitted through the secondary serial interface.
 */
void ConsoleHandler::forward()
{
    const int byte{Serial.read()};
    if (byte != -1)
    {
        ESP_LOGV("TX", "0x%X", byte);
        if (lengthTx == 0U)
        {
            lengthTx = static_cast<size_t>(static_cast<uint8_t>(byte) >> 4U);    // NOLINT(hicpp-signed-bitwise)
            commandTx = static_cast<Command>(static_cast<uint8_t>(byte) & 0xFU); // NOLINT(hicpp-signed-bitwise)
            bytesTx = 0U;
        }
        bufferTx.at(bytesTx++) = static_cast<uint8_t>(byte);
        if (bytesTx == lengthTx + 1U)
        {
            write(std::span{bufferTx}.subspan(0U, lengthTx + 1U));
            lengthTx = 0U;
        }
    }
}

/**
 * @brief Sends a command without an associated value.
 *
 * @param command Command to transmit.
 */
void ConsoleHandler::send(Command command)
{
    const std::array<uint8_t, 1U> payload{static_cast<uint8_t>(command)};
    write(payload);
}

/**
 * @brief Sends a command with a 16-bit value.
 *
 * @param command Command to transmit.
 * @param value Value associated with the command.
 */
void ConsoleHandler::send(Command command, uint16_t value)
{
    const std::array<uint8_t, 3U> payload{
        static_cast<uint8_t>((2U << 4U) | static_cast<uint8_t>(command)),
        static_cast<uint8_t>(value & 0xFFU),
        static_cast<uint8_t>(value >> 8U),
    };
    write(payload);
}

/**
 * @brief Sends a command with two little-endian 16-bit values.
 *
 * @param command Command to transmit.
 * @param value1 First value associated with the command.
 * @param value2 Second value associated with the command.
 */
void ConsoleHandler::send(Command command, uint16_t value1, uint16_t value2)
{
    const std::array<uint8_t, 5U> payload{
        static_cast<uint8_t>((4U << 4U) | static_cast<uint8_t>(command)),
        static_cast<uint8_t>(value1 & 0xFFU),
        static_cast<uint8_t>(value1 >> 8U),
        static_cast<uint8_t>(value2 & 0xFFU),
        static_cast<uint8_t>(value2 >> 8U),
    };
    write(payload);
}

/**
 * @brief Transmits a framed payload through the secondary serial interface.
 *
 * @param payload Bytes to record and transmit.
 */
void ConsoleHandler::write(std::span<const uint8_t> payload)
{
    desk.setTx(payload);
    StatusHandler::setWhite();
    for (const uint8_t byte : payload)
    {
        Serial1.write(byte);
    }
}

#endif // ARDUINO_ARCH_ESP32
