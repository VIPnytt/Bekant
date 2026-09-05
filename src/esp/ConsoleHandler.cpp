#ifdef ARDUINO_ARCH_ESP32

#include "esp/ConsoleHandler.h"

#include "esp/DeviceService.h"
#include "esp/secrets.h"

#include <charconv>

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
        const size_t length{rxBuffer.size()};
        if (byte == static_cast<int>('\n') && length != 0U)
        {
            length >= 2U && length <= 0b1U << 3U ? parse(rxBuffer) : device.statusRed();
            rxBuffer.clear();
        }
        else if (byte != static_cast<int>('\n') && length <= 0b1U << 3U)
        {
            rxBuffer += static_cast<char>(byte);
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
 * @param payload Command containing an encoder value, preset value, button state, or version string.
 * Invalid commands set the device status to red.
 */
void ConsoleHandler::parse(std::string_view payload)
{
    ESP_LOGD("RX", "%.*s", static_cast<int>(payload.size()), payload.data());
    device.setRx(payload);
    const char first{payload.at(0U)};
    if (first == static_cast<char>(0x8U) && payload.size() == 4U)
    {
        device.setEncoder8(static_cast<uint16_t>(payload.at(1U)) | static_cast<uint16_t>(payload.at(2U)) << 8U);
        device.setState8(static_cast<uint8_t>(payload.at(3U)));
        return;
    }
    else if (first == static_cast<char>(0x9U) && payload.size() == 4U)
    {
        device.setEncoder9(static_cast<uint16_t>(payload.at(1U)) | static_cast<uint16_t>(payload.at(2U)) << 8U);
        device.setState9(static_cast<uint8_t>(payload.at(3U)));
        return;
    }
    else if (first == 'v')
    {
        device.setVersion(payload.substr(1U));
        return;
    }
    uint16_t value{}; // NOLINT(misc-const-correctness)
    const std::from_chars_result result{std::from_chars(payload.data() + 1U, payload.data() + payload.size(), value)};
    if (result.ec == std::errc{} && result.ptr == payload.data() + payload.size())
    {
        switch (first) // NOLINT(hicpp-multiway-paths-covered)
        {
        case 'd':
            device.setButtonDown(value == 1U);
            return;
        case 'h':
            device.setPresetHigh(value);
            return;
        case 'l':
            device.setPresetLow(value);
            return;
        case 'u':
            device.setButtonUp(value == 1U);
            return;
        }
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
