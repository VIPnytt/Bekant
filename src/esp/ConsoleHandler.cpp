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
 * @brief Processes serial input, completed messages, and UART receive errors.
 *
 * Forwards primary-serial input when no secondary-serial data or receive error is available.
 */
void ConsoleHandler::handle()
{
    const int byte{Serial1.read()};
    if (byte != -1)
    {
        ESP_LOGV("RX", "0x%X", byte);
        if (rxLength == 0U)
        {
            const uint8_t length{static_cast<uint8_t>(static_cast<uint8_t>(byte) >> 4U)};
            rxLength = length;
            rxCommand = static_cast<uint8_t>(byte) & 0x0FU;
            rxBytes = 0U;
        }
        rxBuffer.at(rxBytes++) = static_cast<uint8_t>(byte);
        if (rxBytes == rxLength + 1U)
        {
            parse();
            rxLength = 0U;
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
 * @brief Forwards a newline-terminated message from the primary serial interface.
 *
 * Carriage returns are ignored, and messages longer than eight characters are discarded.
 */
void ConsoleHandler::forward()
{
    const int byte{Serial.read()};
    if (byte != -1)
    {
        ESP_LOGV("TX", "0x%X", byte);
        const size_t length{txBuffer.size()};
        if (byte == static_cast<int>('\n') && length != 0U)
        {
            if (length <= 0b1U << 3U)
            {
                send(txBuffer);
            }
            txBuffer.clear();
        }
        else if (byte != static_cast<int>('\n') && byte != static_cast<int>('\r') && length <= 0b1U << 3U)
        {
            txBuffer += static_cast<char>(byte);
        }
    }
}

/**
 * @brief Interprets a console payload and updates the corresponding device state.
 *
 * @param payload Binary encoder/state data, a version string, or a numeric button or preset command.
 * Invalid or malformed payloads set the device status to red.
 */
void ConsoleHandler::parse()
{
    ESP_LOGD("RX", "%.*s", static_cast<int>(rxLength + 1U), rxBuffer.data());
    device.setRx(std::span{rxBuffer}.subspan(0U, static_cast<size_t>(1U + rxLength)));
    if (rxCommand == static_cast<uint8_t>(Command::BUTTON_DOWN) && rxLength == 1U)
    {
        device.setButtonDown(static_cast<bool>(rxBuffer.at(1U)));
        return;
    }
    if (rxCommand == static_cast<uint8_t>(Command::BUTTON_UP) && rxLength == 1U)
    {
        device.setButtonUp(static_cast<bool>(rxBuffer.at(1U)));
        return;
    }
    if (rxCommand == static_cast<uint8_t>(Command::ENCODER8) && rxLength == 2U)
    {
        device.setEncoder8(static_cast<uint16_t>(rxBuffer.at(1U)) |
                           static_cast<uint16_t>(static_cast<uint16_t>(rxBuffer.at(2U)) << 8U));
        return;
    }
    if (rxCommand == static_cast<uint8_t>(Command::ENCODER9) && rxLength == 2U)
    {
        device.setEncoder9(static_cast<uint16_t>(rxBuffer.at(1U)) |
                           static_cast<uint16_t>(static_cast<uint16_t>(rxBuffer.at(2U)) << 8U));
        return;
    }
    if (rxCommand == static_cast<uint8_t>(Command::NODE8) && rxLength == 3U)
    {
        device.setEncoder8(static_cast<uint16_t>(rxBuffer.at(1U)) |
                           static_cast<uint16_t>(static_cast<uint16_t>(rxBuffer.at(2U)) << 8U));
        device.setState8(rxBuffer.at(3U));
        return;
    }
    if (rxCommand == static_cast<uint8_t>(Command::NODE9) && rxLength == 3U)
    {
        device.setEncoder9(static_cast<uint16_t>(rxBuffer.at(1U)) |
                           static_cast<uint16_t>(static_cast<uint16_t>(rxBuffer.at(2U)) << 8U));
        device.setState9(rxBuffer.at(3U));
        return;
    }
    if (rxCommand == static_cast<uint8_t>(Command::PRESET_HIGH) && rxLength == 2U)
    {
        device.setPresetHigh(static_cast<uint16_t>(rxBuffer.at(1U)) |
                             static_cast<uint16_t>(static_cast<uint16_t>(rxBuffer.at(2U)) << 8U));
        return;
    }
    if (rxCommand == static_cast<uint8_t>(Command::PRESET_LOW) && rxLength == 2U)
    {
        device.setPresetLow(static_cast<uint16_t>(rxBuffer.at(1U)) |
                            static_cast<uint16_t>(static_cast<uint16_t>(rxBuffer.at(2U)) << 8U));
        return;
    }
    if (rxCommand == static_cast<uint8_t>(Command::STATE8) && rxLength == 1U)
    {
        device.setState8(rxBuffer.at(1U));
        return;
    }
    if (rxCommand == static_cast<uint8_t>(Command::STATE9) && rxLength == 1U)
    {
        device.setState9(rxBuffer.at(1U));
        return;
    }
    if (rxCommand == static_cast<uint8_t>(Command::VERSION))
    {
        device.setVersion(std::span{rxBuffer}.subspan(1U, rxLength));
        return;
    }
    device.statusRed();
}

/**
 * @brief Transmits a newline-terminated payload over the secondary serial interface.
 *
 * @param payload Message to transmit without the terminating newline.
 */
void ConsoleHandler::send(std::string_view payload)
{
    ESP_LOGD("TX", "%.*s", static_cast<int>(payload.size()), payload.data());
    Serial1.write(payload.data(), payload.size());
    Serial1.write('\n');
    device.setTx(payload);
}

/**
 * @brief Stores the latest hardware serial error for processing.
 *
 * @param error Hardware serial error to store.
 */
void ConsoleHandler::onReceiveError(hardwareSerial_error_t error) { lastError = error; }

#endif // ARDUINO_ARCH_ESP32
