#ifdef ARDUINO_ARCH_ESP32

#include "esp/ConsoleHandler.h"

#include "esp/DeviceService.h"
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
        if (stateLength == 0U)
        {
            stateLength = static_cast<size_t>(static_cast<uint8_t>(byte) >> 4U);
            state = static_cast<State>(static_cast<uint8_t>(byte) & 0x0FU);
            stateBytes = 0U;
        }
        stateBuffer.at(stateBytes++) = static_cast<uint8_t>(byte);
        if (stateBytes == stateLength + 1U)
        {
            parse();
            stateLength = 0U;
        }
    }
    else if (lastError != hardwareSerial_error_t::UART_NO_ERROR)
    {
        const uint8_t _error{static_cast<uint8_t>(lastError)};
        lastError = hardwareSerial_error_t::UART_NO_ERROR;
        ESP_LOGW("hardwareSerial_error_t", "%d", _error);
        device.statusRed();
        JsonDocument doc{};
        doc["hardwareSerial_error_t"].set(_error);
        device.transmit(doc);
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
        if (commandLength == 0U)
        {
            commandLength = static_cast<size_t>(static_cast<uint8_t>(byte) >> 4U);
            command = static_cast<Command>(static_cast<uint8_t>(byte) & 0x0FU);
            commandBytes = 0U;
        }
        commandBuffer.at(commandBytes++) = static_cast<uint8_t>(byte);
        if (commandBytes == commandLength + 1U)
        {
            write(std::span{commandBuffer}.subspan(0U, commandLength + 1U));
            commandLength = 0U;
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
    device.setRx(std::span{stateBuffer}.subspan(0U, stateLength + 1U));
    if (state == State::BUTTON_DOWN && stateLength == 1U)
    {
        device.setButtonDown(static_cast<bool>(stateBuffer.at(1U)));
        return;
    }
    if (state == State::BUTTON_UP && stateLength == 1U)
    {
        device.setButtonUp(static_cast<bool>(stateBuffer.at(1U)));
        return;
    }
    if (state == State::ENCODER8 && stateLength == 2U)
    {
        device.setEncoder8(static_cast<uint16_t>(stateBuffer.at(1U)) |
                           static_cast<uint16_t>(static_cast<uint16_t>(stateBuffer.at(2U)) << 8U));
        return;
    }
    if (state == State::ENCODER9 && stateLength == 2U)
    {
        device.setEncoder9(static_cast<uint16_t>(stateBuffer.at(1U)) |
                           static_cast<uint16_t>(static_cast<uint16_t>(stateBuffer.at(2U)) << 8U));
        return;
    }
    if (state == State::NODE8 && stateLength == 3U)
    {
        device.setEncoder8(static_cast<uint16_t>(stateBuffer.at(1U)) |
                           static_cast<uint16_t>(static_cast<uint16_t>(stateBuffer.at(2U)) << 8U));
        device.setState8(stateBuffer.at(3U));
        return;
    }
    if (state == State::NODE9 && stateLength == 3U)
    {
        device.setEncoder9(static_cast<uint16_t>(stateBuffer.at(1U)) |
                           static_cast<uint16_t>(static_cast<uint16_t>(stateBuffer.at(2U)) << 8U));
        device.setState9(stateBuffer.at(3U));
        return;
    }
    if (state == State::PRESET_HIGH && stateLength == 2U)
    {
        device.setPresetHigh(static_cast<uint16_t>(stateBuffer.at(1U)) |
                             static_cast<uint16_t>(static_cast<uint16_t>(stateBuffer.at(2U)) << 8U));
        return;
    }
    if (state == State::PRESET_LOW && stateLength == 2U)
    {
        device.setPresetLow(static_cast<uint16_t>(stateBuffer.at(1U)) |
                            static_cast<uint16_t>(static_cast<uint16_t>(stateBuffer.at(2U)) << 8U));
        return;
    }
    if (state == State::STATE8 && stateLength == 1U)
    {
        device.setState8(stateBuffer.at(1U));
        return;
    }
    if (state == State::STATE9 && stateLength == 1U)
    {
        device.setState9(stateBuffer.at(1U));
        return;
    }
    device.statusRed();
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
    device.setTx(payload);
    device.statusWhite();
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
void ConsoleHandler::onReceiveError(hardwareSerial_error_t error) { lastError = error; }

#endif // ARDUINO_ARCH_ESP32
