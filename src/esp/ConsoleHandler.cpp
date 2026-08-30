#ifdef ARDUINO_ARCH_ESP32

#include "esp/ConsoleHandler.h"

#include "esp/DeviceService.h"
#include "esp/secrets.h"

void ConsoleHandler::begin()
{
    pinMode(PIN_MISO, INPUT);
    pinMode(PIN_SCK, OUTPUT);
    Serial1.onReceiveError(&onReceiveError);
    Serial1.begin(115'200UL, SerialConfig::SERIAL_8N1, PIN_MISO, PIN_SCK);
}

/**
 * @brief Processes one byte from the serial input and handles completed messages or serial errors.
 */
void ConsoleHandler::handle()
{
    const int byte{Serial1.read()};
    if (byte != -1)
    {
        ESP_LOGV("RX", "0x%X", byte);
        if (byte == static_cast<int>('\n'))
        {
            ESP_LOGD("RX", "%s", buffer.c_str());
            parse(buffer);
            buffer.clear();
        }
        else
        {
            buffer += static_cast<char>(byte);
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
}

void ConsoleHandler::parse(std::string message)
{
    device.setRx(message);
    const char first{message.at(0U)};
    if (first == 'a' && (message.size() == 4U || message.size() == 5U))
    {
        device.setEncoderA(static_cast<uint16_t>(atoi(message.substr(1U).c_str())));
    }
    else if (first == 'b' && (message.size() == 4U || message.size() == 5U))
    {
        device.setEncoderB(static_cast<uint16_t>(atoi(message.substr(1U).c_str())));
    }
    else if (first == 'd' && message.size() == 2U)
    {
        device.setButtonDown(message.at(1U) == '1');
    }
    else if (first == 'h' && (message.size() == 4U || message.size() == 5U))
    {
        device.setPresetHigh(static_cast<uint16_t>(atoi(message.substr(1U).c_str())));
    }
    else if (first == 'l' && (message.size() == 4U || message.size() == 5U))
    {
        device.setPresetLow(static_cast<uint16_t>(atoi(message.substr(1U).c_str())));
    }
    else if (first == 'u' && message.size() == 2U)
    {
        device.setButtonUp(message.at(1U) == '1');
    }
    else
    {
        device.statusRed();
    }
}

void ConsoleHandler::send(std::string payload)
{
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
