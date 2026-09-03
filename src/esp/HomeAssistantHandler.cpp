#ifdef ARDUINO_ARCH_ESP32

#include "esp/HomeAssistantHandler.h"

#include "esp/DeviceService.h" // NOLINT(misc-include-cleaner)
#include "esp/constants.h"

#include <WiFi.h>
#include <format>

/**
 * @brief Configures the Home Assistant availability topic and unavailable payload.
 */
void HomeAssistantHandler::availability()
{
    JsonObject _availability{discovery[ComponentAbbreviations::availability].to<JsonObject>()};
    _availability[ComponentAbbreviations::payload_not_available].set("");
    _availability[ComponentAbbreviations::topic].set("bekant/" HOSTNAME "/availability");
}

/**
 * @brief Populates the Home Assistant discovery document with all entity groups.
 */
void HomeAssistantHandler::components()
{
    controls();
    sensors();
    configuration();
    diagnostic();
}

/**
 * @brief Adds IKEA BEKANT desk device metadata to the Home Assistant discovery document.
 */
void HomeAssistantHandler::device()
{
    JsonObject _device{discovery[ComponentAbbreviations::device].to<JsonObject>()};
    _device[DeviceAbbreviations::connections][0U][0U].set("mac");
    _device[DeviceAbbreviations::connections][0U][1U].set(WiFi.macAddress());
    _device[DeviceAbbreviations::hw_version].set(ARDUINO_BOARD);
    _device[DeviceAbbreviations::identifiers][0U].set(std::format("0x{:x}", ESP.getEfuseMac()));
    _device[DeviceAbbreviations::manufacturer].set("IKEA");
    _device[DeviceAbbreviations::model].set("BEKANT");
    _device[DeviceAbbreviations::name].set(NAME);
    _device[DeviceAbbreviations::sw_version].set(std::string("Bekant ").append(DeviceService::version));
}

/**
 * @brief Configures the Home Assistant discovery origin metadata.
 */
void HomeAssistantHandler::origin()
{
    JsonObject _origin{discovery[ComponentAbbreviations::origin].to<JsonObject>()};
    _origin[OriginAbbreviations::name].set("Bekant");
    _origin[OriginAbbreviations::support_url].set("https://github.com/VIPnytt/Bekant");
}

/**
 * @brief Configures Home Assistant controls for desk height and preset recall.
 */
void HomeAssistantHandler::controls()
{
    {
        JsonObject height{discovery[ComponentAbbreviations::components]["height"].to<JsonObject>()};
        height[ComponentAbbreviations::command_template].set(R"({"desk":{{value}}})");
        height[ComponentAbbreviations::command_topic].set(commandTopic);
        height[ComponentAbbreviations::device_class].set("distance");
        height[ComponentAbbreviations::icon].set("mdi:desk");
        height[ComponentAbbreviations::json_attributes_template].set(
            R"({"Encoders":{{value_json.encoders}},"Legs":{{value_json.legs}}})");
        height[ComponentAbbreviations::json_attributes_topic].set(stateTopic);
        height[ComponentAbbreviations::max].set(ReferenceHeight::heightHigh);
        height[ComponentAbbreviations::min].set(ReferenceHeight::heightLow);
        height[ComponentAbbreviations::mode].set("box");
        height[ComponentAbbreviations::name].set("Height");
        height[ComponentAbbreviations::platform].set("number");
        height[ComponentAbbreviations::state_topic].set(stateTopic);
        height[ComponentAbbreviations::step].set(.1F);
        height[ComponentAbbreviations::unique_id].set("height");
        height[ComponentAbbreviations::unit_of_measurement].set(ReferenceHeight::heightUnit);
        height[ComponentAbbreviations::value_template].set("{{value_json.desk|round(1)}}");
    }
    {
        JsonObject presetHigh{discovery[ComponentAbbreviations::components]["recall_high"].to<JsonObject>()};
        presetHigh[ComponentAbbreviations::command_template].set(R"({"preset":{"{{value}}":true}})");
        presetHigh[ComponentAbbreviations::command_topic].set(commandTopic);
        presetHigh[ComponentAbbreviations::icon].set("mdi:menu-up-outline");
        presetHigh[ComponentAbbreviations::json_attributes_template].set(R"({"Preset":{{value_json.preset.high}}})");
        presetHigh[ComponentAbbreviations::json_attributes_topic].set(stateTopic);
        presetHigh[ComponentAbbreviations::name].set("Preset high");
        presetHigh[ComponentAbbreviations::payload_press].set("high");
        presetHigh[ComponentAbbreviations::platform].set("button");
        presetHigh[ComponentAbbreviations::unique_id].set("recall_high");
    }
    {
        JsonObject presetLow{discovery[ComponentAbbreviations::components]["recall_low"].to<JsonObject>()};
        presetLow[ComponentAbbreviations::command_template].set(R"({"preset":{"{{value}}":true}})");
        presetLow[ComponentAbbreviations::command_topic].set(commandTopic);
        presetLow[ComponentAbbreviations::icon].set("mdi:menu-down-outline");
        presetLow[ComponentAbbreviations::json_attributes_template].set(R"({"Preset":{{value_json.preset.low}}})");
        presetLow[ComponentAbbreviations::json_attributes_topic].set(stateTopic);
        presetLow[ComponentAbbreviations::name].set("Preset low");
        presetLow[ComponentAbbreviations::payload_press].set("low");
        presetLow[ComponentAbbreviations::platform].set("button");
        presetLow[ComponentAbbreviations::unique_id].set("recall_low");
    }
}

/**
 * @brief Configures Home Assistant sensors for desk height and stored presets.
 */
void HomeAssistantHandler::sensors()
{
    {
        JsonObject desk{discovery[ComponentAbbreviations::components]["desk"].to<JsonObject>()};
        desk[ComponentAbbreviations::device_class].set("distance");
        desk[ComponentAbbreviations::icon].set("mdi:desk");
        desk[ComponentAbbreviations::json_attributes_template].set(
            R"({"Encoders":{{value_json.encoders}},"Legs":{{value_json.legs}}})");
        desk[ComponentAbbreviations::json_attributes_topic].set(stateTopic);
        desk[ComponentAbbreviations::name].set("Desk");
        desk[ComponentAbbreviations::platform].set("sensor");
        desk[ComponentAbbreviations::state_class].set("measurement");
        desk[ComponentAbbreviations::state_topic].set(stateTopic);
        desk[ComponentAbbreviations::suggested_display_precision].set(1U);
        desk[ComponentAbbreviations::unique_id].set("desk");
        desk[ComponentAbbreviations::unit_of_measurement].set(ReferenceHeight::heightUnit);
        desk[ComponentAbbreviations::value_template].set(R"({{value_json.desk}})");
    }
    {
        JsonObject presetHigh{discovery[ComponentAbbreviations::components]["high_sensor"].to<JsonObject>()};
        presetHigh[ComponentAbbreviations::device_class].set("distance");
        presetHigh[ComponentAbbreviations::icon].set("mdi:menu-up-outline");
        presetHigh[ComponentAbbreviations::name].set("Preset high");
        presetHigh[ComponentAbbreviations::platform].set("sensor");
        presetHigh[ComponentAbbreviations::state_class].set("measurement");
        presetHigh[ComponentAbbreviations::state_topic].set(stateTopic);
        presetHigh[ComponentAbbreviations::suggested_display_precision].set(1U);
        presetHigh[ComponentAbbreviations::unique_id].set("high_sensor");
        presetHigh[ComponentAbbreviations::unit_of_measurement].set(ReferenceHeight::heightUnit);
        presetHigh[ComponentAbbreviations::value_template].set(R"({{value_json.preset.high|round(1)}})");
    }
    {
        JsonObject presetLow{discovery[ComponentAbbreviations::components]["low_sensor"].to<JsonObject>()};
        presetLow[ComponentAbbreviations::device_class].set("distance");
        presetLow[ComponentAbbreviations::icon].set("mdi:menu-down-outline");
        presetLow[ComponentAbbreviations::name].set("Preset low");
        presetLow[ComponentAbbreviations::platform].set("sensor");
        presetLow[ComponentAbbreviations::state_class].set("measurement");
        presetLow[ComponentAbbreviations::state_topic].set(stateTopic);
        presetLow[ComponentAbbreviations::suggested_display_precision].set(1U);
        presetLow[ComponentAbbreviations::unique_id].set("low_sensor");
        presetLow[ComponentAbbreviations::unit_of_measurement].set(ReferenceHeight::heightUnit);
        presetLow[ComponentAbbreviations::value_template].set(R"({{value_json.preset.low|round(1)}})");
    }
}

/**
 * @brief Configures Home Assistant entities for desk settings and maintenance actions.
 */
void HomeAssistantHandler::configuration()
{
    constexpr std::string_view entityCategory{"config"};
#ifdef PIN_OE
    {
        JsonObject outputEnable{discovery[ComponentAbbreviations::components]["oe"].to<JsonObject>()};
        outputEnable[ComponentAbbreviations::command_template].set(R"({"oe":{{value}}})");
        outputEnable[ComponentAbbreviations::command_topic].set(commandTopic);
        outputEnable[ComponentAbbreviations::entity_category].set(entityCategory);
        outputEnable[ComponentAbbreviations::icon].set("mdi:chip");
        outputEnable[ComponentAbbreviations::name].set("Output enable");
        outputEnable[ComponentAbbreviations::payload_off].set("false");
        outputEnable[ComponentAbbreviations::payload_on].set("true");
        outputEnable[ComponentAbbreviations::state_off].set("False");
        outputEnable[ComponentAbbreviations::state_on].set("True");
        outputEnable[ComponentAbbreviations::platform].set("switch");
        outputEnable[ComponentAbbreviations::state_topic].set(stateTopic);
        outputEnable[ComponentAbbreviations::unique_id].set("oe");
        outputEnable[ComponentAbbreviations::value_template].set("{{value_json.oe}}");
    }
#endif // PIN_OE
    {
        JsonObject presetHigh{discovery[ComponentAbbreviations::components]["preset_high"].to<JsonObject>()};
        presetHigh[ComponentAbbreviations::command_template].set(R"({"preset":{"high":{{value}}}})");
        presetHigh[ComponentAbbreviations::command_topic].set(commandTopic);
        presetHigh[ComponentAbbreviations::device_class].set("distance");
        presetHigh[ComponentAbbreviations::entity_category].set(entityCategory);
        presetHigh[ComponentAbbreviations::icon].set("mdi:menu-up-outline");
        presetHigh[ComponentAbbreviations::max].set(ReferenceHeight::heightHigh);
        presetHigh[ComponentAbbreviations::min].set(ReferenceHeight::heightLow);
        presetHigh[ComponentAbbreviations::mode].set("box");
        presetHigh[ComponentAbbreviations::name].set("Preset high");
        presetHigh[ComponentAbbreviations::platform].set("number");
        presetHigh[ComponentAbbreviations::state_topic].set(stateTopic);
        presetHigh[ComponentAbbreviations::step].set(.1F);
        presetHigh[ComponentAbbreviations::unique_id].set("preset_high");
        presetHigh[ComponentAbbreviations::unit_of_measurement].set(ReferenceHeight::heightUnit);
        presetHigh[ComponentAbbreviations::value_template].set("{{value_json.preset.high|round(1)}}");
    }
    {
        JsonObject presetLow{discovery[ComponentAbbreviations::components]["preset_low"].to<JsonObject>()};
        presetLow[ComponentAbbreviations::command_template].set(R"({"preset":{"low":{{value}}}})");
        presetLow[ComponentAbbreviations::command_topic].set(commandTopic);
        presetLow[ComponentAbbreviations::device_class].set("distance");
        presetLow[ComponentAbbreviations::entity_category].set(entityCategory);
        presetLow[ComponentAbbreviations::icon].set("mdi:menu-down-outline");
        presetLow[ComponentAbbreviations::max].set(ReferenceHeight::heightHigh);
        presetLow[ComponentAbbreviations::min].set(ReferenceHeight::heightLow);
        presetLow[ComponentAbbreviations::mode].set("box");
        presetLow[ComponentAbbreviations::name].set("Preset low");
        presetLow[ComponentAbbreviations::platform].set("number");
        presetLow[ComponentAbbreviations::state_topic].set(stateTopic);
        presetLow[ComponentAbbreviations::step].set(.1F);
        presetLow[ComponentAbbreviations::unique_id].set("preset_low");
        presetLow[ComponentAbbreviations::unit_of_measurement].set(ReferenceHeight::heightUnit);
        presetLow[ComponentAbbreviations::value_template].set("{{value_json.preset.low|round(1)}}");
    }
    {
        JsonObject reboot{discovery[ComponentAbbreviations::components]["reboot"].to<JsonObject>()};
        reboot[ComponentAbbreviations::command_template].set(R"({"action":"{{value}}"})");
        reboot[ComponentAbbreviations::command_topic].set(commandTopic);
        reboot[ComponentAbbreviations::device_class].set("restart");
        reboot[ComponentAbbreviations::enabled_by_default].set(false);
        reboot[ComponentAbbreviations::entity_category].set(entityCategory);
        reboot[ComponentAbbreviations::name].set("Reboot");
        reboot[ComponentAbbreviations::payload_press].set("restart");
        reboot[ComponentAbbreviations::platform].set("button");
        reboot[ComponentAbbreviations::unique_id].set("reboot");
    }
    {
        JsonObject reset{discovery[ComponentAbbreviations::components]["reset"].to<JsonObject>()};
        reset[ComponentAbbreviations::command_template].set(R"({"reset":{{value}}})");
        reset[ComponentAbbreviations::command_topic].set(commandTopic);
        reset[ComponentAbbreviations::enabled_by_default].set(false);
        reset[ComponentAbbreviations::entity_category].set(entityCategory);
        reset[ComponentAbbreviations::icon].set("mdi:lock-outline");
        reset[ComponentAbbreviations::name].set("Reset");
        reset[ComponentAbbreviations::payload_off].set("false");
        reset[ComponentAbbreviations::payload_on].set("true");
        reset[ComponentAbbreviations::state_off].set("False");
        reset[ComponentAbbreviations::state_on].set("True");
        reset[ComponentAbbreviations::platform].set("switch");
        reset[ComponentAbbreviations::state_topic].set(stateTopic);
        reset[ComponentAbbreviations::unique_id].set("reset");
        reset[ComponentAbbreviations::value_template].set("{{value_json.reset}}");
    }
}

/**
 * @brief Configures diagnostic entities for Home Assistant discovery.
 *
 * Adds diagnostic controls and sensors for calibration, encoder data, firmware
 * versions, positional offset, serial activity, temperature, Wi-Fi signal
 * strength, and optionally button inputs and power-supply voltage. Diagnostic
 * entities are categorized and selected hardware-specific entities are disabled
 * by default.
 */
void HomeAssistantHandler::diagnostic()
{
    constexpr std::string_view entityCategory{"diagnostic"};
#ifdef PIN_TPDN
    {
        JsonObject buttonDown{discovery[ComponentAbbreviations::components]["tpdn"].to<JsonObject>()};
        buttonDown[ComponentAbbreviations::command_template].set(R"({"button":{"down":{{value}}}})");
        buttonDown[ComponentAbbreviations::command_topic].set(commandTopic);
        buttonDown[ComponentAbbreviations::enabled_by_default].set(false);
        buttonDown[ComponentAbbreviations::entity_category].set(entityCategory);
        buttonDown[ComponentAbbreviations::icon].set("mdi:menu-down-outline");
        buttonDown[ComponentAbbreviations::name].set("Button down");
        buttonDown[ComponentAbbreviations::payload_off].set("false");
        buttonDown[ComponentAbbreviations::payload_on].set("true");
        buttonDown[ComponentAbbreviations::state_off].set("False");
        buttonDown[ComponentAbbreviations::state_on].set("True");
        buttonDown[ComponentAbbreviations::platform].set("switch");
        buttonDown[ComponentAbbreviations::state_topic].set(stateTopic);
        buttonDown[ComponentAbbreviations::unique_id].set("tpdn");
        buttonDown[ComponentAbbreviations::value_template].set("{{value_json.button.down}}");
    }
#endif // PIN_TPDN
#ifdef PIN_TPUP
    {
        JsonObject buttonUp{discovery[ComponentAbbreviations::components]["tpup"].to<JsonObject>()};
        buttonUp[ComponentAbbreviations::command_template].set(R"({"button":{"up":{{value}}}})");
        buttonUp[ComponentAbbreviations::command_topic].set(commandTopic);
        buttonUp[ComponentAbbreviations::enabled_by_default].set(false);
        buttonUp[ComponentAbbreviations::entity_category].set(entityCategory);
        buttonUp[ComponentAbbreviations::icon].set("mdi:menu-up-outline");
        buttonUp[ComponentAbbreviations::name].set("Button up");
        buttonUp[ComponentAbbreviations::payload_off].set("false");
        buttonUp[ComponentAbbreviations::payload_on].set("true");
        buttonUp[ComponentAbbreviations::state_off].set("False");
        buttonUp[ComponentAbbreviations::state_on].set("True");
        buttonUp[ComponentAbbreviations::platform].set("switch");
        buttonUp[ComponentAbbreviations::state_topic].set(stateTopic);
        buttonUp[ComponentAbbreviations::unique_id].set("tpup");
        buttonUp[ComponentAbbreviations::value_template].set("{{value_json.button.up}}");
    }
#endif // PIN_TPUP
    {
        JsonObject calibrate{discovery[ComponentAbbreviations::components]["calibrate"].to<JsonObject>()};
        calibrate[ComponentAbbreviations::command_template].set(R"({"action":"{{value}}"})");
        calibrate[ComponentAbbreviations::command_topic].set(commandTopic);
        calibrate[ComponentAbbreviations::entity_category].set(entityCategory);
        calibrate[ComponentAbbreviations::icon].set("mdi:arrow-collapse-down");
        calibrate[ComponentAbbreviations::name].set("Calibrate");
        calibrate[ComponentAbbreviations::payload_press].set("calibrate");
        calibrate[ComponentAbbreviations::platform].set("button");
        calibrate[ComponentAbbreviations::unique_id].set("calibrate");
    }
    {
        JsonObject firmwareAvr{discovery[ComponentAbbreviations::components]["firmware_avr"].to<JsonObject>()};
        firmwareAvr[ComponentAbbreviations::entity_category].set(entityCategory);
        firmwareAvr[ComponentAbbreviations::icon].set("mdi:update");
        firmwareAvr[ComponentAbbreviations::name].set("Firmware AVR");
        firmwareAvr[ComponentAbbreviations::platform].set("update");
        firmwareAvr[ComponentAbbreviations::release_url].set(
            std::string("https://github.com/VIPnytt/Bekant/releases/v").append(DeviceService::version));
        firmwareAvr[ComponentAbbreviations::state_topic].set(stateTopic);
        firmwareAvr[ComponentAbbreviations::title].set("Bekant AVR firmware");
        firmwareAvr[ComponentAbbreviations::unique_id].set("firmware_avr");
        firmwareAvr[ComponentAbbreviations::value_template].set(
            std::string("{{{'installed_version':value_json.firmware.avr,'latest_version':'")
                .append(DeviceService::version)
                .append("'}|to_json}}"));
    }
    {
        JsonObject firmwareEsp32{discovery[ComponentAbbreviations::components]["firmware_esp32"].to<JsonObject>()};
        firmwareEsp32[ComponentAbbreviations::enabled_by_default].set(false);
        firmwareEsp32[ComponentAbbreviations::entity_category].set(entityCategory);
        firmwareEsp32[ComponentAbbreviations::icon].set("mdi:update");
        firmwareEsp32[ComponentAbbreviations::name].set("Firmware ESP32");
        firmwareEsp32[ComponentAbbreviations::platform].set("update");
        firmwareEsp32[ComponentAbbreviations::release_url].set("https://github.com/VIPnytt/Bekant/releases/latest");
        firmwareEsp32[ComponentAbbreviations::state_topic].set(stateTopic);
        firmwareEsp32[ComponentAbbreviations::title].set("Bekant ESP32 firmware");
        firmwareEsp32[ComponentAbbreviations::unique_id].set("firmware_esp32");
        firmwareEsp32[ComponentAbbreviations::value_template].set(
            "{{{'installed_version':value_json.firmware.esp32,'latest_version':value_json.firmware.latest}|to_json}}");
    }
    {
        JsonObject offset{discovery[ComponentAbbreviations::components]["offset"].to<JsonObject>()};
        offset[ComponentAbbreviations::device_class].set("distance");
        offset[ComponentAbbreviations::enabled_by_default].set(false);
        offset[ComponentAbbreviations::entity_category].set(entityCategory);
        offset[ComponentAbbreviations::icon].set("mdi:align-vertical-top");
        offset[ComponentAbbreviations::json_attributes_template].set(
            R"({"Encoders":{{value_json.encoders}},"Legs":{{value_json.legs}}})");
        offset[ComponentAbbreviations::json_attributes_topic].set(stateTopic);
        offset[ComponentAbbreviations::name].set("Offset");
        offset[ComponentAbbreviations::platform].set("sensor");
        offset[ComponentAbbreviations::state_class].set("measurement");
        offset[ComponentAbbreviations::state_topic].set(stateTopic);
        offset[ComponentAbbreviations::suggested_display_precision].set(1U);
        offset[ComponentAbbreviations::unique_id].set("offset");
        offset[ComponentAbbreviations::unit_of_measurement].set(ReferenceHeight::heightUnit);
        offset[ComponentAbbreviations::value_template].set("{{value_json.offset}}");
    }
    {
        JsonObject position{discovery[ComponentAbbreviations::components]["encoders"].to<JsonObject>()};
        position[ComponentAbbreviations::entity_category].set(entityCategory);
        position[ComponentAbbreviations::icon].set("mdi:counter");
        position[ComponentAbbreviations::json_attributes_template].set(R"({"raw":{{value_json.encoders}}})");
        position[ComponentAbbreviations::json_attributes_topic].set(stateTopic);
        position[ComponentAbbreviations::name].set("Position");
        position[ComponentAbbreviations::platform].set("sensor");
        position[ComponentAbbreviations::state_class].set("measurement");
        position[ComponentAbbreviations::state_topic].set(stateTopic);
        position[ComponentAbbreviations::suggested_display_precision].set(0U);
        position[ComponentAbbreviations::unique_id].set("encoders");
        position[ComponentAbbreviations::value_template].set(
            R"({{value_json.encoders|sum/value_json.encoders|length}})");
    }
#ifdef PIN_ADC
    {
        JsonObject powerSupply{discovery[ComponentAbbreviations::components]["adc"].to<JsonObject>()};
        powerSupply[ComponentAbbreviations::device_class].set("voltage");
        powerSupply[ComponentAbbreviations::enabled_by_default].set(false);
        powerSupply[ComponentAbbreviations::entity_category].set(entityCategory);
        powerSupply[ComponentAbbreviations::icon].set("mdi:alpha-v-circle-outline");
        powerSupply[ComponentAbbreviations::name].set("Power supply");
        powerSupply[ComponentAbbreviations::suggested_display_precision].set(1);
        powerSupply[ComponentAbbreviations::platform].set("sensor");
        powerSupply[ComponentAbbreviations::state_class].set("measurement");
        powerSupply[ComponentAbbreviations::state_topic].set(stateTopic);
        powerSupply[ComponentAbbreviations::unique_id].set("adc");
        powerSupply[ComponentAbbreviations::unit_of_measurement].set("V");
        powerSupply[ComponentAbbreviations::value_template].set("{{value_json.voltage}}");
    }
#endif // PIN_ADC
    {
        JsonObject serialRx{discovery[ComponentAbbreviations::components]["rx"].to<JsonObject>()};
        serialRx[ComponentAbbreviations::enabled_by_default].set(false);
        serialRx[ComponentAbbreviations::entity_category].set(entityCategory);
        serialRx[ComponentAbbreviations::icon].set("mdi:message-cog-outline");
        serialRx[ComponentAbbreviations::name].set("Serial RX");
        serialRx[ComponentAbbreviations::platform].set("sensor");
        serialRx[ComponentAbbreviations::state_topic].set(stateTopic);
        serialRx[ComponentAbbreviations::unique_id].set("rx");
        serialRx[ComponentAbbreviations::value_template].set("{{value_json.rx}}");
    }
    {
        JsonObject serialTx{discovery[ComponentAbbreviations::components]["tx"].to<JsonObject>()};
        serialTx[ComponentAbbreviations::enabled_by_default].set(false);
        serialTx[ComponentAbbreviations::entity_category].set(entityCategory);
        serialTx[ComponentAbbreviations::icon].set("mdi:email-arrow-right");
        serialTx[ComponentAbbreviations::name].set("Serial TX");
        serialTx[ComponentAbbreviations::platform].set("sensor");
        serialTx[ComponentAbbreviations::state_topic].set(stateTopic);
        serialTx[ComponentAbbreviations::unique_id].set("tx");
        serialTx[ComponentAbbreviations::value_template].set("{{value_json.tx}}");
    }
    {
        JsonObject temperature{discovery[ComponentAbbreviations::components]["temperature"].to<JsonObject>()};
        temperature[ComponentAbbreviations::device_class].set("temperature");
        temperature[ComponentAbbreviations::enabled_by_default].set(false);
        temperature[ComponentAbbreviations::entity_category].set(entityCategory);
        temperature[ComponentAbbreviations::name].set("Temperature");
        temperature[ComponentAbbreviations::platform].set("sensor");
        temperature[ComponentAbbreviations::state_class].set("measurement");
        temperature[ComponentAbbreviations::state_topic].set(stateTopic);
        temperature[ComponentAbbreviations::unique_id].set("temperature");
        temperature[ComponentAbbreviations::unit_of_measurement].set("°C");
        temperature[ComponentAbbreviations::value_template].set("{{value_json.temperature}}");
    }
    {
        JsonObject wifiSignal{discovery[ComponentAbbreviations::components]["rssi"].to<JsonObject>()};
        wifiSignal[ComponentAbbreviations::device_class].set("signal_strength");
        wifiSignal[ComponentAbbreviations::entity_category].set(entityCategory);
        wifiSignal[ComponentAbbreviations::name].set("Wi-Fi signal");
        wifiSignal[ComponentAbbreviations::platform].set("sensor");
        wifiSignal[ComponentAbbreviations::state_class].set("measurement");
        wifiSignal[ComponentAbbreviations::state_topic].set(stateTopic);
        wifiSignal[ComponentAbbreviations::unique_id].set("rssi");
        wifiSignal[ComponentAbbreviations::unit_of_measurement].set("dBm");
        wifiSignal[ComponentAbbreviations::value_template].set("{{value_json.rssi}}");
    }
}

#endif // ARDUINO_ARCH_ESP32
