#ifdef ARDUINO_ARCH_ESP32

#include "esp/ConsoleHandler.h"

#include "esp/DeskService.h"
#include "esp/secrets.h"

#include <string_view>

/**
 * @brief Initializes the serial console and configures its communication pins.
 */
void ConsoleHandler::begin()
{
    pinMode(PIN_MISO, INPUT);
    pinMode(PIN_SCK, OUTPUT);
    Serial1.onReceiveError(&onReceiveError);
    Serial1.begin(115'200UL, SerialConfig::SERIAL_8N1, PIN_MISO, PIN_SCK);
}

/**
 * @brief Processes available secondary-serial data, pending UART errors, or primary-serial input.
 */
void ConsoleHandler::handle()
{
    const int byte{Serial1.read()};
    if (byte != -1)
    {
        ESP_LOGV("RX", "0x%X", byte);
        if (lengthRx == 0U)
        {
            lengthRx = static_cast<size_t>(static_cast<uint8_t>(byte) >> 4U);
            stateRx = static_cast<State>(static_cast<uint8_t>(byte) & 0xFU);
            bytesRx = 0U;
        }
        bufferRx.at(bytesRx++) = static_cast<uint8_t>(byte);
        if (bytesRx == lengthRx + 1U)
        {
            parse();
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
            lengthTx = static_cast<size_t>(static_cast<uint8_t>(byte) >> 4U);
            commandTx = static_cast<Command>(static_cast<uint8_t>(byte) & 0xFU);
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
 * @brief Applies the buffered console frame to the corresponding device state.
 *
 * Invalid command and payload-length combinations set the device status to red.
 */
void ConsoleHandler::parse() const
{
    desk.setRx(std::span{bufferRx}.subspan(0U, lengthRx + 1U));
    if (stateRx == State::BUTTON_DOWN && lengthRx == 1U)
    {
        desk.setButtonDown(static_cast<bool>(bufferRx.at(1U)));
    }
    else if (stateRx == State::BUTTON_UP && lengthRx == 1U)
    {
        desk.setButtonUp(static_cast<bool>(bufferRx.at(1U)));
    }
    else if (stateRx == State::CONSOLE && lengthRx == 1U)
    {
        desk.setErrorTx(bufferRx.at(1U));
    }
    else if (stateRx == State::INITIALIZATION)
    {
        desk.setErrorAvr();
    }
    else if (stateRx == State::LIN && lengthRx == 1U)
    {
        desk.setErrorLin(bufferRx.at(1U));
    }
    else if (stateRx == State::NODE8)
    {
        lengthRx == 3U
            ? desk.setNode8(static_cast<uint16_t>(static_cast<uint16_t>(bufferRx.at(1U)) |
                                                  static_cast<uint16_t>(static_cast<uint16_t>(bufferRx.at(2U)) << 8U)),
                            bufferRx.at(3U))
            : desk.setErrorNode8();
    }
    else if (stateRx == State::NODE9)
    {
        lengthRx == 3U
            ? desk.setNode9(static_cast<uint16_t>(static_cast<uint16_t>(bufferRx.at(1U)) |
                                                  static_cast<uint16_t>(static_cast<uint16_t>(bufferRx.at(2U)) << 8U)),
                            bufferRx.at(3U))
            : desk.setErrorNode9();
    }
    else if (stateRx == State::PRESET_HIGH && lengthRx == 2U)
    {
        desk.setPresetHigh(static_cast<uint16_t>(bufferRx.at(1U)) |
                           static_cast<uint16_t>(static_cast<uint16_t>(bufferRx.at(2U)) << 8U));
    }
    else if (stateRx == State::PRESET_LOW && lengthRx == 2U)
    {
        desk.setPresetLow(static_cast<uint16_t>(bufferRx.at(1U)) |
                          static_cast<uint16_t>(static_cast<uint16_t>(bufferRx.at(2U)) << 8U));
    }
    else
    {
        desk.statusRed();
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
 * @brief Transmits a framed payload through the secondary serial interface.
 *
 * @param payload Bytes to record and transmit.
 */
void ConsoleHandler::write(std::span<const uint8_t> payload)
{
    desk.setTx(payload);
    desk.statusWhite();
    for (const uint8_t byte : payload)
    {
        Serial1.write(byte);
    }
}

/**
 * @brief Stores the latest hardware serial error for processing.
 *
 * @param error Hardware serial error to store.
 */
void ConsoleHandler::onReceiveError(hardwareSerial_error_t error)
{
    ESP_LOGW("hardwareSerial_error_t", "%u", static_cast<unsigned int>(error));
    desk.setErrorRx(error);
}

#endif // ARDUINO_ARCH_ESP32
