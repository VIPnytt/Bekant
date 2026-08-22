#pragma once

#ifdef ARDUINO_ARCH_ESP32

#include "esp/secrets.h"

#include <ArduinoJson.h>
#include <HardwareSerial.h>
#include <string>

class DeskService
{
private:
    bool buttonDown{false};
    bool buttonUp{false};

    std::pair<uint16_t, float> legA{0U, .0F};
    std::pair<uint16_t, float> legB{0U, .0F};
    std::pair<uint16_t, float> presetLow{0U, .0F};
    std::pair<uint16_t, float> presetHigh{0U, .0F};

    std::string buffer{};

    static inline hardwareSerial_error_t lastError{hardwareSerial_error_t::UART_NO_ERROR};

    void decode(std::pair<uint16_t, float> &height, uint16_t encoded);
    void parse(const std::string message);

    static void onReceiveError(hardwareSerial_error_t error);

public:
    void begin();
    void handle();
    void metadata(JsonDocument &doc);
    void save();

    void onHomeAssistant(JsonDocument &doc);

    static DeskService &getInstance();
};

extern DeskService &Desk;

#endif // ARDUINO_ARCH_ESP32