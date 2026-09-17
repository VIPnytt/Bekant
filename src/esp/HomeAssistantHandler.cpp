#ifdef ARDUINO_ARCH_ESP32

#include "esp/HomeAssistantHandler.h"

#include "esp/DeskService.h" // NOLINT(misc-include-cleaner)
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
    _device[DeviceAbbreviations::sw_version].set(std::string("Bekant ").append(DeskService::version));
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
 * @brief Configures Home Assistant controls for height, preset recall, and optional up/down simulation.
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
            R"({"Encoders":{{value_json.encoders}},"Legs":{{value_json.legs}},"States":{{value_json.states}}})");
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
#ifdef PIN_TPDN
    {
        JsonObject lower{discovery[ComponentAbbreviations::components]["lower"].to<JsonObject>()};
        lower[ComponentAbbreviations::command_template].set(R"({"simulate":{"down":{{value}}}})");
        lower[ComponentAbbreviations::command_topic].set(commandTopic);
        lower[ComponentAbbreviations::enabled_by_default].set(false);
        lower[ComponentAbbreviations::icon].set("mdi:menu-down-outline");
        lower[ComponentAbbreviations::name].set("Lower");
        lower[ComponentAbbreviations::payload_off].set("false");
        lower[ComponentAbbreviations::payload_on].set("true");
        lower[ComponentAbbreviations::state_off].set("False");
        lower[ComponentAbbreviations::state_on].set("True");
        lower[ComponentAbbreviations::platform].set("switch");
        lower[ComponentAbbreviations::state_topic].set(stateTopic);
        lower[ComponentAbbreviations::unique_id].set("lower");
        lower[ComponentAbbreviations::value_template].set("{{value_json.simulate.down}}");
    }
#endif // PIN_TPDN
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
#ifdef PIN_TPUP
    {
        JsonObject raise{discovery[ComponentAbbreviations::components]["raise"].to<JsonObject>()};
        raise[ComponentAbbreviations::command_template].set(R"({"simulate":{"up":{{value}}}})");
        raise[ComponentAbbreviations::command_topic].set(commandTopic);
        raise[ComponentAbbreviations::enabled_by_default].set(false);
        raise[ComponentAbbreviations::icon].set("mdi:menu-up-outline");
        raise[ComponentAbbreviations::name].set("Raise");
        raise[ComponentAbbreviations::payload_off].set("false");
        raise[ComponentAbbreviations::payload_on].set("true");
        raise[ComponentAbbreviations::state_off].set("False");
        raise[ComponentAbbreviations::state_on].set("True");
        raise[ComponentAbbreviations::platform].set("switch");
        raise[ComponentAbbreviations::state_topic].set(stateTopic);
        raise[ComponentAbbreviations::unique_id].set("raise");
        raise[ComponentAbbreviations::value_template].set("{{value_json.simulate.up}}");
    }
#endif // PIN_TPUP
    {
        JsonObject tone{discovery[ComponentAbbreviations::components]["tone"].to<JsonObject>()};
        tone[ComponentAbbreviations::command_template].set(R"({"tone":{}})");
        tone[ComponentAbbreviations::command_topic].set(commandTopic);
        tone[ComponentAbbreviations::enabled_by_default].set(false);
        tone[ComponentAbbreviations::icon].set("mdi:music-note-outline");
        tone[ComponentAbbreviations::json_attributes_template].set(
            R"({"Duration":{{value_json.tone.duration}},"Frequency":{{value_json.tone.frequency}}})");
        tone[ComponentAbbreviations::json_attributes_topic].set(stateTopic);
        tone[ComponentAbbreviations::name].set("Tone");
        tone[ComponentAbbreviations::platform].set("button");
        tone[ComponentAbbreviations::unique_id].set("tone");
    }
}

/**
 * @brief Configures Home Assistant sensors for physical button states, desk height, and stored presets.
 */
void HomeAssistantHandler::sensors()
{
    {
        JsonObject button3{discovery[ComponentAbbreviations::components]["button_3"].to<JsonObject>()};
        button3[ComponentAbbreviations::device_class].set("occupancy");
        button3[ComponentAbbreviations::enabled_by_default].set(false);
        button3[ComponentAbbreviations::icon].set("mdi:numeric-3-circle-outline");
        button3[ComponentAbbreviations::name].set("Button 3");
        button3[ComponentAbbreviations::payload_off].set("False");
        button3[ComponentAbbreviations::payload_on].set("True");
        button3[ComponentAbbreviations::platform].set("binary_sensor");
        button3[ComponentAbbreviations::state_topic].set(stateTopic);
        button3[ComponentAbbreviations::unique_id].set("button_3");
        button3[ComponentAbbreviations::value_template].set("{{value_json.button['3']}}");
    }
    {
        JsonObject button4{discovery[ComponentAbbreviations::components]["button_4"].to<JsonObject>()};
        button4[ComponentAbbreviations::device_class].set("occupancy");
        button4[ComponentAbbreviations::enabled_by_default].set(false);
        button4[ComponentAbbreviations::icon].set("mdi:numeric-4-circle-outline");
        button4[ComponentAbbreviations::name].set("Button 4");
        button4[ComponentAbbreviations::payload_off].set("False");
        button4[ComponentAbbreviations::payload_on].set("True");
        button4[ComponentAbbreviations::platform].set("binary_sensor");
        button4[ComponentAbbreviations::state_topic].set(stateTopic);
        button4[ComponentAbbreviations::unique_id].set("button_4");
        button4[ComponentAbbreviations::value_template].set("{{value_json.button['4']}}");
    }
    {
        JsonObject buttonDown{discovery[ComponentAbbreviations::components]["button_down"].to<JsonObject>()};
        buttonDown[ComponentAbbreviations::device_class].set("occupancy");
        buttonDown[ComponentAbbreviations::enabled_by_default].set(false);
        buttonDown[ComponentAbbreviations::icon].set("mdi:menu-down-outline");
        buttonDown[ComponentAbbreviations::name].set("Button down");
        buttonDown[ComponentAbbreviations::payload_off].set("False");
        buttonDown[ComponentAbbreviations::payload_on].set("True");
        buttonDown[ComponentAbbreviations::platform].set("binary_sensor");
        buttonDown[ComponentAbbreviations::state_topic].set(stateTopic);
        buttonDown[ComponentAbbreviations::unique_id].set("button_down");
        buttonDown[ComponentAbbreviations::value_template].set("{{value_json.button.down}}");
    }
    {
        JsonObject buttonUp{discovery[ComponentAbbreviations::components]["button_up"].to<JsonObject>()};
        buttonUp[ComponentAbbreviations::device_class].set("occupancy");
        buttonUp[ComponentAbbreviations::enabled_by_default].set(false);
        buttonUp[ComponentAbbreviations::icon].set("mdi:menu-up-outline");
        buttonUp[ComponentAbbreviations::name].set("Button up");
        buttonUp[ComponentAbbreviations::payload_off].set("False");
        buttonUp[ComponentAbbreviations::payload_on].set("True");
        buttonUp[ComponentAbbreviations::platform].set("binary_sensor");
        buttonUp[ComponentAbbreviations::state_topic].set(stateTopic);
        buttonUp[ComponentAbbreviations::unique_id].set("button_up");
        buttonUp[ComponentAbbreviations::value_template].set("{{value_json.button.up}}");
    }
    {
        JsonObject desk{discovery[ComponentAbbreviations::components]["desk"].to<JsonObject>()};
        desk[ComponentAbbreviations::device_class].set("distance");
        desk[ComponentAbbreviations::icon].set("mdi:desk");
        desk[ComponentAbbreviations::json_attributes_template].set(
            R"({"Encoders":{{value_json.encoders}},"Legs":{{value_json.legs}},"States":{{value_json.states}}})");
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
        presetHigh[ComponentAbbreviations::value_template].set("{{value_json.preset.high|round(1)}}");
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
        presetLow[ComponentAbbreviations::value_template].set("{{value_json.preset.low|round(1)}}");
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
        JsonObject recalibrateLegs{discovery[ComponentAbbreviations::components]["recalibrate"].to<JsonObject>()};
        recalibrateLegs[ComponentAbbreviations::command_template].set(R"({"action":"{{value}}"})");
        recalibrateLegs[ComponentAbbreviations::command_topic].set(commandTopic);
        recalibrateLegs[ComponentAbbreviations::entity_category].set(entityCategory);
        recalibrateLegs[ComponentAbbreviations::icon].set("mdi:sync-alert");
        recalibrateLegs[ComponentAbbreviations::name].set("Recalibrate legs");
        recalibrateLegs[ComponentAbbreviations::payload_press].set("recalibrate");
        recalibrateLegs[ComponentAbbreviations::platform].set("button");
        recalibrateLegs[ComponentAbbreviations::unique_id].set("recalibrate");
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
    {
        JsonObject toneDuration{discovery[ComponentAbbreviations::components]["tone_duration"].to<JsonObject>()};
        toneDuration[ComponentAbbreviations::command_template].set(R"({"tone":{"duration":{{(value*1000)|int}}}})");
        toneDuration[ComponentAbbreviations::command_topic].set(commandTopic);
        toneDuration[ComponentAbbreviations::device_class].set("duration");
        toneDuration[ComponentAbbreviations::enabled_by_default].set(false);
        toneDuration[ComponentAbbreviations::entity_category].set(entityCategory);
        toneDuration[ComponentAbbreviations::icon].set("mdi:timer-music-outline");
        toneDuration[ComponentAbbreviations::max].set(8U);
        toneDuration[ComponentAbbreviations::min].set(.1F);
        toneDuration[ComponentAbbreviations::mode].set("box");
        toneDuration[ComponentAbbreviations::name].set("Tone duration");
        toneDuration[ComponentAbbreviations::platform].set("number");
        toneDuration[ComponentAbbreviations::state_topic].set(stateTopic);
        toneDuration[ComponentAbbreviations::step].set(.1F);
        toneDuration[ComponentAbbreviations::unique_id].set("tone_duration");
        toneDuration[ComponentAbbreviations::unit_of_measurement].set("s");
        toneDuration[ComponentAbbreviations::value_template].set("{{(value_json.tone.duration/1000)|round(1)}}");
    }
    {
        JsonObject toneFrequency{discovery[ComponentAbbreviations::components]["tone_frequency"].to<JsonObject>()};
        toneFrequency[ComponentAbbreviations::command_template].set(R"({"tone":{"frequency":{{(value*1000)|int}}}})");
        toneFrequency[ComponentAbbreviations::command_topic].set(commandTopic);
        toneFrequency[ComponentAbbreviations::device_class].set("frequency");
        toneFrequency[ComponentAbbreviations::enabled_by_default].set(false);
        toneFrequency[ComponentAbbreviations::entity_category].set(entityCategory);
        toneFrequency[ComponentAbbreviations::icon].set("mdi:sine-wave");
        toneFrequency[ComponentAbbreviations::max].set(8U);
        toneFrequency[ComponentAbbreviations::min].set(.1F);
        toneFrequency[ComponentAbbreviations::mode].set("slider");
        toneFrequency[ComponentAbbreviations::name].set("Tone frequency");
        toneFrequency[ComponentAbbreviations::platform].set("number");
        toneFrequency[ComponentAbbreviations::state_topic].set(stateTopic);
        toneFrequency[ComponentAbbreviations::step].set(.1F);
        toneFrequency[ComponentAbbreviations::unique_id].set("tone_frequency");
        toneFrequency[ComponentAbbreviations::unit_of_measurement].set("kHz");
        toneFrequency[ComponentAbbreviations::value_template].set("{{(value_json.tone.frequency/1000)|round(1)}}");
    }
}

/**
 * @brief Configures diagnostic entities for Home Assistant discovery.
 *
 * Adds diagnostic entities for encoder data, firmware versions, device issues,
 * positional offset, serial activity,
 * temperature, Wi-Fi signal strength, and optional power-supply voltage.
 * Diagnostic entities are categorized and selected hardware-specific entities
 * are disabled by default.
 */
void HomeAssistantHandler::diagnostic()
{
    constexpr std::string_view entityCategory{"diagnostic"};
    {
        JsonObject firmware{discovery[ComponentAbbreviations::components]["firmware"].to<JsonObject>()};
        firmware[ComponentAbbreviations::enabled_by_default].set(false);
        firmware[ComponentAbbreviations::entity_category].set(entityCategory);
        firmware[ComponentAbbreviations::icon].set("mdi:update");
        firmware[ComponentAbbreviations::name].set("Firmware");
        firmware[ComponentAbbreviations::platform].set("update");
        firmware[ComponentAbbreviations::release_url].set("https://github.com/VIPnytt/Bekant/releases/latest");
        firmware[ComponentAbbreviations::state_topic].set(stateTopic);
        firmware[ComponentAbbreviations::title].set("Bekant");
        firmware[ComponentAbbreviations::unique_id].set("firmware");
        firmware[ComponentAbbreviations::value_template].set(
            "{{{'installed_version':value_json.version.installed,'latest_version':value_json.version.latest}|to_json}}");
    }
    {
        JsonObject issues{discovery[ComponentAbbreviations::components]["issues"].to<JsonObject>()};
        issues[ComponentAbbreviations::entity_category].set(entityCategory);
        issues[ComponentAbbreviations::icon].set("mdi:alert-outline");
        issues[ComponentAbbreviations::name].set("Issues");
        issues[ComponentAbbreviations::platform].set("sensor");
        issues[ComponentAbbreviations::state_topic].set(stateTopic);
        issues[ComponentAbbreviations::unique_id].set("issues");
        issues[ComponentAbbreviations::value_template].set("{{value_json.issues|sort|join(', ')}}");
    }
    {
        JsonObject offset{discovery[ComponentAbbreviations::components]["offset"].to<JsonObject>()};
        offset[ComponentAbbreviations::device_class].set("distance");
        offset[ComponentAbbreviations::enabled_by_default].set(false);
        offset[ComponentAbbreviations::entity_category].set(entityCategory);
        offset[ComponentAbbreviations::icon].set("mdi:align-vertical-top");
        offset[ComponentAbbreviations::json_attributes_template].set(
            R"({"Encoders":{{value_json.encoders}},"Legs":{{value_json.legs}},"States":{{value_json.states}}})");
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
        position[ComponentAbbreviations::json_attributes_template].set(
            R"({"Encoders":{{value_json.encoders}},"Legs":{{value_json.legs}},"States":{{value_json.states}}})");
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
        serialRx[ComponentAbbreviations::value_template].set(
            R"({%for i in range(0,value_json.rx|length,2)%}{{value_json.rx[i:i+2]}} {%endfor%})");
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
        serialTx[ComponentAbbreviations::value_template].set(
            R"({%for i in range(0,value_json.tx|length,2)%}{{value_json.tx[i:i+2]}} {%endfor%})");
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
