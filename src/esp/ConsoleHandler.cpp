#ifdef ARDUINO_ARCH_ESP32

#include "esp/ConsoleHandler.h"

#include "esp/DeviceService.h"
#include "esp/secrets.h"

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
 * @brief Processes one byte from the serial input and handles completed messages or serial errors.
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
            if (length <= 0b1U << 3U)
            {
                parse(rxBuffer);
            }
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
 * @brief Parses a console command and updates the corresponding device state.
 *
 * @param message Command containing an encoder value, preset value, or button state.
 */
void ConsoleHandler::parse(std::string payload)
{
    ESP_LOGD("RX", "%s", rxBuffer.c_str());
    device.setRx(payload);
    const char first{payload.at(0U)};
    if (first == 'a' && (payload.size() == 4U || payload.size() == 5U))
    {
        device.setEncoderA(static_cast<uint16_t>(atoi(payload.substr(1U).c_str())));
    }
    else if (first == 'b' && (payload.size() == 4U || payload.size() == 5U))
    {
        device.setEncoderB(static_cast<uint16_t>(atoi(payload.substr(1U).c_str())));
    }
    else if (first == 'd' && payload.size() == 2U)
    {
        device.setButtonDown(payload.at(1U) == '1');
    }
    else if (first == 'h' && (payload.size() == 4U || payload.size() == 5U))
    {
        device.setPresetHigh(static_cast<uint16_t>(atoi(payload.substr(1U).c_str())));
    }
    else if (first == 'l' && (payload.size() == 4U || payload.size() == 5U))
    {
        device.setPresetLow(static_cast<uint16_t>(atoi(payload.substr(1U).c_str())));
    }
    else if (first == 'u' && payload.size() == 2U)
    {
        device.setButtonUp(payload.at(1U) == '1');
    }
    else
    {
        device.statusRed();
    }
}

/**
 * @brief Transmits a payload over the serial console.
 *
 * @param payload Message to transmit without the terminating newline.
 */
void ConsoleHandler::send(std::string payload)
{
    ESP_LOGD("TX", "%s", payload.c_str());
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
