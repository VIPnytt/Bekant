#pragma once

#ifdef ARDUINO_ARCH_ESP32

#include <ArduinoJson.h> // NOLINT(misc-include-cleaner)
#include <HardwareSerial.h>
#include <string>

class DeskHandler
{
private:
    bool buttonDown{false};
    bool buttonUp{false};
    bool saved{true};

    std::pair<uint16_t, float> legA{0U, .0F};
    std::pair<uint16_t, float> legB{0U, .0F};
    std::pair<uint16_t, float> presetLow{0U, .0F};
    std::pair<uint16_t, float> presetHigh{0U, .0F};

    std::string buffer{};

    static inline hardwareSerial_error_t lastError{hardwareSerial_error_t::UART_NO_ERROR};

    void decode(std::pair<uint16_t, float> &height, uint16_t encoded);
    void parse(std::string message);
    void parseButton(bool &button, bool state);
    void parseEncoder(std::pair<uint16_t, float> &leg, uint16_t encoded);
    void parsePreset(std::pair<uint16_t, float> &preset, uint16_t encoded);

    static void onReceiveError(hardwareSerial_error_t error);

public:
    void begin();
    void handle();
    void metadata(JsonDocument &doc);
    void save();

    void onHomeAssistant(JsonDocument &doc);
};

#endif // ARDUINO_ARCH_ESP32