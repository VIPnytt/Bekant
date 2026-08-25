#ifdef ARDUINO_ARCH_ESP32

#include "esp/DeskHandler.h"

#include "esp/DeviceService.h"
#include "esp/constants.h"

#include <nvs.h>

/**
 * @brief Loads persisted desk settings and initializes the serial interface.
 */
void DeskHandler::begin()
{
    nvs_handle_t handle{};
    if (nvs_open("bekant", nvs_open_mode_t::NVS_READONLY, &handle) == ESP_OK)
    {
        uint16_t value{};
        if (nvs_get_u16(handle, "a", &value) == ESP_OK)
        {
            decode(legA, value);
        }
        if (nvs_get_u16(handle, "b", &value) == ESP_OK)
        {
            decode(legB, value);
        }
        if (nvs_get_u16(handle, "h", &value) == ESP_OK)
        {
            decode(presetHigh, value);
        }
        if (nvs_get_u16(handle, "l", &value) == ESP_OK)
        {
            decode(presetLow, value);
        }
        nvs_close(handle);
    }
    Serial1.onReceiveError(&onReceiveError);
    Serial1.begin(115'200UL, SerialConfig::SERIAL_8N1, PIN_MISO, PIN_SCK);
}

/**
 * @brief Processes one byte from the serial input and handles completed messages or serial errors.
 */
void DeskHandler::handle()
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
        device.statusRed();
        const uint8_t _error{static_cast<uint8_t>(lastError)};
        lastError = hardwareSerial_error_t::UART_NO_ERROR;
        ESP_LOGW("hardwareSerial_error_t", "%d", _error);
        JsonDocument doc{};
        doc["hardwareSerial_error_t"].set(_error);
        device.transmit(doc);
    }
}

/**
 * @brief Processes a desk controller message and publishes the resulting state.
 *
 * Recognized messages update leg encoder values, button states, or height presets.
 * Invalid messages set the device status to red. Each message is included with
 * current metadata in the transmitted document.
 *
 * @param message Newline-delimited desk controller message.
 */
void DeskHandler::parse(std::string message)
{
    JsonDocument doc{};
    const char first{message.at(0U)};
    if (first == 'a' && (message.size() == 4U || message.size() == 5U))
    {
        parseEncoder(legA, static_cast<uint16_t>(atoi(message.substr(1U).c_str())));
    }
    else if (first == 'b' && (message.size() == 4U || message.size() == 5U))
    {
        parseEncoder(legB, static_cast<uint16_t>(atoi(message.substr(1U).c_str())));
    }
    else if (first == 'd' && message.size() == 2U)
    {
        parseButton(buttonDown, message.at(1U) == '1');
    }
    else if (first == 'h' && (message.size() == 4U || message.size() == 5U))
    {
        parsePreset(presetHigh, static_cast<uint16_t>(atoi(message.substr(1U).c_str())));
    }
    else if (first == 'l' && (message.size() == 4U || message.size() == 5U))
    {
        parsePreset(presetLow, static_cast<uint16_t>(atoi(message.substr(1U).c_str())));
    }
    else if (first == 'u' && message.size() == 2U)
    {
        parseButton(buttonUp, message.at(1U) == '1');
    }
    else
    {
        device.statusRed();
    }
    metadata(doc);
    doc["rx"].set(message);
    device.transmit(doc);
}

/**
 * @brief Updates a button state and sets the device status based on button activity.
 *
 * @param button Button state to update.
 * @param state New button state.
 */
void DeskHandler::parseButton(bool &button, bool state) const
{
    button = state;
    buttonDown || buttonUp ? device.statusGreen() : device.statusWhite();
}

/**
 * @brief Updates a leg's decoded height when its encoded value changes.
 *
 * @param leg Encoded value and decoded height for the leg.
 * @param encoded New encoded leg value.
 */
void DeskHandler::parseEncoder(std::pair<uint16_t, float> &leg, uint16_t encoded)
{
    if (encoded != leg.first)
    {
        decode(leg, encoded);
        saved = false;
        buttonDown || buttonUp ? device.statusGreen() : device.statusBlue();
    }
}

/**
 * @brief Updates a preset when its encoded value changes.
 *
 * @param preset Preset value and its encoded representation.
 * @param encoded New encoded preset value.
 */
void DeskHandler::parsePreset(std::pair<uint16_t, float> &preset, uint16_t encoded)
{
    if (encoded != preset.first)
    {
        decode(preset, encoded);
        saved = false;
        device.statusWhite();
    }
}

/**
 * @brief Persists unsaved encoder and preset values to non-volatile storage.
 */
void DeskHandler::save()
{
    if (!saved)
    {
        nvs_handle_t handle{};
        if (nvs_open("bekant", nvs_open_mode_t::NVS_READWRITE, &handle) == ESP_OK)
        {
            nvs_set_u16(handle, "a", legA.first);
            nvs_set_u16(handle, "b", legB.first);
            nvs_set_u16(handle, "h", presetHigh.first);
            nvs_set_u16(handle, "l", presetLow.first);
            saved = nvs_commit(handle) == ESP_OK;
            nvs_close(handle);
        }
    }
}

/**
 * @brief Adds desk controls, measurements, encoder values, leg heights, and presets to a JSON document.
 *
 * @param doc JSON document to populate.
 */
void DeskHandler::metadata(JsonDocument &doc)
{
    doc["button"]["down"].set(buttonDown);
    doc["button"]["up"].set(buttonUp);
    if (legA.first != 0U && legB.first != 0U)
    {
        doc["desk"].set((legA.second + legB.second) / 2.0F);
    }
    if (legA.first != 0U)
    {
        doc["encoders"][0U].set(legA.first);
        doc["legs"][0U].set(legA.second);
    }
    if (legB.first != 0U)
    {
        doc["encoders"][1U].set(legB.first);
        doc["legs"][1U].set(legB.second);
    }
    if (presetHigh.first != 0U)
    {
        doc["preset"]["high"].set(presetHigh.second);
    }
    if (presetLow.first != 0U)
    {
        doc["preset"]["low"].set(presetLow.second);
    }
}

/**
 * @brief Decodes an encoder value into a physical desk height.
 *
 * @param height Pair to populate with the raw encoder value and calculated height.
 * @param encoded Encoded 16-bit encoder value.
 */
void DeskHandler::decode(std::pair<uint16_t, float> &height, uint16_t encoded)
{
    height.first = encoded;
    height.second = ((static_cast<float>(height.first) - static_cast<float>(ReferenceHeight::encoderLow)) *
                     (ReferenceHeight::heightHigh - ReferenceHeight::heightLow) /
                     static_cast<float>(ReferenceHeight::encoderHigh - ReferenceHeight::encoderLow)) +
                    ReferenceHeight::heightLow;
}

/**
 * @brief Adds Home Assistant MQTT discovery configurations for desk controls, measurements, presets, and serial
 * diagnostics.
 *
 * @param doc JSON document to populate with the Home Assistant component definitions.
 */
void DeskHandler::onHomeAssistant(JsonDocument &doc)
{
    {
        JsonObject desk{doc[HomeAssistantAbbreviations::components]["desk"].to<JsonObject>()};
        desk[HomeAssistantAbbreviations::device_class].set("distance");
        desk[HomeAssistantAbbreviations::icon].set("mdi:desk");
        desk[HomeAssistantAbbreviations::json_attributes_template].set(
            R"({"Encoders":{{value_json.encoders}},"Legs":{{value_json.legs}}})");
        desk[HomeAssistantAbbreviations::json_attributes_topic].set("bekant/" HOSTNAME "/state");
        desk[HomeAssistantAbbreviations::name].set("Desk");
        desk[HomeAssistantAbbreviations::platform].set("sensor");
        desk[HomeAssistantAbbreviations::state_class].set("measurement");
        desk[HomeAssistantAbbreviations::state_topic].set("bekant/" HOSTNAME "/state");
        desk[HomeAssistantAbbreviations::suggested_display_precision].set(1U);
        desk[HomeAssistantAbbreviations::unique_id].set("desk");
        desk[HomeAssistantAbbreviations::unit_of_measurement].set(ReferenceHeight::heightUnit);
        desk[HomeAssistantAbbreviations::value_template].set(R"({{value_json.desk}})");
    }
    {
        JsonObject encoders{doc[HomeAssistantAbbreviations::components]["encoders"].to<JsonObject>()};
        encoders[HomeAssistantAbbreviations::entity_category].set("diagnostic");
        encoders[HomeAssistantAbbreviations::icon].set("mdi:counter");
        encoders[HomeAssistantAbbreviations::json_attributes_template].set(R"({"raw":{{value_json.encoders}}})");
        encoders[HomeAssistantAbbreviations::json_attributes_topic].set("bekant/" HOSTNAME "/state");
        encoders[HomeAssistantAbbreviations::name].set("Encoders");
        encoders[HomeAssistantAbbreviations::platform].set("sensor");
        encoders[HomeAssistantAbbreviations::state_class].set("measurement");
        encoders[HomeAssistantAbbreviations::state_topic].set("bekant/" HOSTNAME "/state");
        encoders[HomeAssistantAbbreviations::suggested_display_precision].set(0U);
        encoders[HomeAssistantAbbreviations::unique_id].set("encoders");
        encoders[HomeAssistantAbbreviations::value_template].set(
            R"({{value_json.encoders|sum/value_json.encoders|length}})");
    }
    {
        JsonObject height{doc[HomeAssistantAbbreviations::components]["height"].to<JsonObject>()};
        height[HomeAssistantAbbreviations::command_template].set(R"({"desk":{{value}}})");
        height[HomeAssistantAbbreviations::command_topic].set("bekant/" HOSTNAME "/set");
        height[HomeAssistantAbbreviations::device_class].set("distance");
        height[HomeAssistantAbbreviations::icon].set("mdi:desk");
        height[HomeAssistantAbbreviations::json_attributes_template].set(
            R"({"Encoders":{{value_json.encoders}},"Legs":{{value_json.legs}}})");
        height[HomeAssistantAbbreviations::json_attributes_topic].set("bekant/" HOSTNAME "/state");
        height[HomeAssistantAbbreviations::max].set(ReferenceHeight::heightHigh);
        height[HomeAssistantAbbreviations::min].set(ReferenceHeight::heightLow);
        height[HomeAssistantAbbreviations::mode].set("box");
        height[HomeAssistantAbbreviations::name].set("Height");
        height[HomeAssistantAbbreviations::platform].set("number");
        height[HomeAssistantAbbreviations::state_topic].set("bekant/" HOSTNAME "/state");
        height[HomeAssistantAbbreviations::step].set(.1F);
        height[HomeAssistantAbbreviations::unique_id].set("height");
        height[HomeAssistantAbbreviations::unit_of_measurement].set(ReferenceHeight::heightUnit);
        height[HomeAssistantAbbreviations::value_template].set("{{value_json.desk|round(1)}}");
    }
    {
        JsonObject highPreset{doc[HomeAssistantAbbreviations::components]["preset_high"].to<JsonObject>()};
        highPreset[HomeAssistantAbbreviations::command_template].set(R"({"preset":{"high":{{value}}}})");
        highPreset[HomeAssistantAbbreviations::command_topic].set("bekant/" HOSTNAME "/set");
        highPreset[HomeAssistantAbbreviations::device_class].set("distance");
        highPreset[HomeAssistantAbbreviations::entity_category].set("config");
        highPreset[HomeAssistantAbbreviations::icon].set("mdi:menu-up-outline");
        highPreset[HomeAssistantAbbreviations::max].set(ReferenceHeight::heightHigh);
        highPreset[HomeAssistantAbbreviations::min].set(ReferenceHeight::heightLow);
        highPreset[HomeAssistantAbbreviations::mode].set("box");
        highPreset[HomeAssistantAbbreviations::name].set("Preset high");
        highPreset[HomeAssistantAbbreviations::platform].set("number");
        highPreset[HomeAssistantAbbreviations::state_topic].set("bekant/" HOSTNAME "/state");
        highPreset[HomeAssistantAbbreviations::step].set(.1F);
        highPreset[HomeAssistantAbbreviations::unique_id].set("preset_high");
        highPreset[HomeAssistantAbbreviations::unit_of_measurement].set(ReferenceHeight::heightUnit);
        highPreset[HomeAssistantAbbreviations::value_template].set("{{value_json.preset.high|round(1)}}");
    }
    {
        JsonObject lowPreset{doc[HomeAssistantAbbreviations::components]["preset_low"].to<JsonObject>()};
        lowPreset[HomeAssistantAbbreviations::command_template].set(R"({"preset":{"low":{{value}}}})");
        lowPreset[HomeAssistantAbbreviations::command_topic].set("bekant/" HOSTNAME "/set");
        lowPreset[HomeAssistantAbbreviations::device_class].set("distance");
        lowPreset[HomeAssistantAbbreviations::entity_category].set("config");
        lowPreset[HomeAssistantAbbreviations::icon].set("mdi:menu-down-outline");
        lowPreset[HomeAssistantAbbreviations::max].set(ReferenceHeight::heightHigh);
        lowPreset[HomeAssistantAbbreviations::min].set(ReferenceHeight::heightLow);
        lowPreset[HomeAssistantAbbreviations::mode].set("box");
        lowPreset[HomeAssistantAbbreviations::name].set("Preset low");
        lowPreset[HomeAssistantAbbreviations::platform].set("number");
        lowPreset[HomeAssistantAbbreviations::state_topic].set("bekant/" HOSTNAME "/state");
        lowPreset[HomeAssistantAbbreviations::step].set(.1F);
        lowPreset[HomeAssistantAbbreviations::unique_id].set("preset_low");
        lowPreset[HomeAssistantAbbreviations::unit_of_measurement].set(ReferenceHeight::heightUnit);
        lowPreset[HomeAssistantAbbreviations::value_template].set("{{value_json.preset.low|round(1)}}");
    }
    {
        JsonObject offset{doc[HomeAssistantAbbreviations::components]["offset"].to<JsonObject>()};
        offset[HomeAssistantAbbreviations::device_class].set("distance");
        offset[HomeAssistantAbbreviations::enabled_by_default].set(false);
        offset[HomeAssistantAbbreviations::entity_category].set("diagnostic");
        offset[HomeAssistantAbbreviations::icon].set("mdi:align-vertical-top");
        offset[HomeAssistantAbbreviations::json_attributes_template].set(
            R"({"Encoders":{{value_json.encoders}},"Legs":{{value_json.legs}}})");
        offset[HomeAssistantAbbreviations::json_attributes_topic].set("bekant/" HOSTNAME "/state");
        offset[HomeAssistantAbbreviations::name].set("Offset");
        offset[HomeAssistantAbbreviations::platform].set("sensor");
        offset[HomeAssistantAbbreviations::state_class].set("measurement");
        offset[HomeAssistantAbbreviations::state_topic].set("bekant/" HOSTNAME "/state");
        offset[HomeAssistantAbbreviations::suggested_display_precision].set(1U);
        offset[HomeAssistantAbbreviations::unique_id].set("offset");
        offset[HomeAssistantAbbreviations::unit_of_measurement].set(ReferenceHeight::heightUnit);
        offset[HomeAssistantAbbreviations::value_template].set("{{value_json.legs[0]-value_json.legs[1]}}");
    }
    {
        JsonObject rx{doc[HomeAssistantAbbreviations::components]["rx"].to<JsonObject>()};
        rx[HomeAssistantAbbreviations::enabled_by_default].set(false);
        rx[HomeAssistantAbbreviations::entity_category].set("diagnostic");
        rx[HomeAssistantAbbreviations::icon].set("mdi:message-cog-outline");
        rx[HomeAssistantAbbreviations::name].set("Serial RX");
        rx[HomeAssistantAbbreviations::platform].set("sensor");
        rx[HomeAssistantAbbreviations::state_topic].set("bekant/" HOSTNAME "/state");
        rx[HomeAssistantAbbreviations::unique_id].set("rx");
        rx[HomeAssistantAbbreviations::value_template].set("{{value_json.rx}}");
    }
    {
        JsonObject tx{doc[HomeAssistantAbbreviations::components]["tx"].to<JsonObject>()};
        tx[HomeAssistantAbbreviations::enabled_by_default].set(false);
        tx[HomeAssistantAbbreviations::entity_category].set("diagnostic");
        tx[HomeAssistantAbbreviations::icon].set("mdi:email-arrow-right");
        tx[HomeAssistantAbbreviations::name].set("Serial TX");
        tx[HomeAssistantAbbreviations::platform].set("sensor");
        tx[HomeAssistantAbbreviations::state_topic].set("bekant/" HOSTNAME "/state");
        tx[HomeAssistantAbbreviations::unique_id].set("tx");
        tx[HomeAssistantAbbreviations::value_template].set("{{value_json.tx}}");
    }
}

/**
 * @brief Stores the latest hardware serial error for processing.
 *
 * @param error Hardware serial error to store.
 */
void DeskHandler::onReceiveError(hardwareSerial_error_t error) { lastError = error; }

#endif // ARDUINO_ARCH_ESP32
