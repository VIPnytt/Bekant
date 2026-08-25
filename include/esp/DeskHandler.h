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

    /**
 * Converts an encoded height value into a height record.
 * @param height Height record to update.
 * @param encoded Encoded height value.
 */

/**
 * Processes a received desk message.
 * @param message Received desk message.
 */

/**
 * Updates a desk button state.
 * @param button Button state to update.
 * @param state New button state.
 */

/**
 * Updates a leg position from an encoded value.
 * @param leg Leg position record to update.
 * @param encoded Encoded leg-position value.
 */

/**
 * Updates a preset height from an encoded value.
 * @param preset Preset record to update.
 * @param encoded Encoded preset value.
 */

/**
 * Records a hardware serial receive error.
 * @param error Hardware serial error.
 */

/**
 * Initializes desk control handling.
 */

/**
 * Processes desk communication and state updates.
 */

/**
 * Adds desk state and metadata to a JSON document.
 * @param doc JSON document to update.
 */

/**
 * Persists the current desk settings.
 */

/**
 * Processes data received through Home Assistant.
 * @param doc JSON document containing the received data.
 */
static inline hardwareSerial_error_t lastError{hardwareSerial_error_t::UART_NO_ERROR};

    void decode(std::pair<uint16_t, float> &height, uint16_t encoded);
    void parse(std::string message);
    void parseButton(bool &button, bool state) const;
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