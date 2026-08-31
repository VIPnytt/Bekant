#pragma once

#ifdef ARDUINO_ARCH_ESP32

#include "esp/secrets.h"

#include <ArduinoJson.h> // NOLINT(misc-include-cleaner)

class HomeAssistantHandler
{
private:
    static constexpr std::string_view commandTopic{"bekant/" HOSTNAME "/set"};
    /**
 * Creates a handler that populates the provided Home Assistant discovery document.
 * @param doc JSON document used to store discovery data.
 */
static constexpr std::string_view stateTopic{"bekant/" HOSTNAME "/state"};

    JsonDocument &discovery; // NOLINT(cppcoreguidelines-avoid-const-or-ref-data-members)

    void configuration();
    void controls();
    void diagnostic();
    void sensors();

public:
    explicit HomeAssistantHandler(JsonDocument &doc) : discovery(doc) {};

    void availability();
    void components();
    void device();
    void origin();
};

namespace ComponentAbbreviations
{
static constexpr std::string_view availability{"avty"};
static constexpr std::string_view command_template{"cmd_tpl"};
static constexpr std::string_view command_topic{"cmd_t"};
static constexpr std::string_view components{"cmps"};
static constexpr std::string_view device{"dev"};
static constexpr std::string_view device_class{"dev_cla"};
static constexpr std::string_view enabled_by_default{"en"};
static constexpr std::string_view entity_category{"ent_cat"};
static constexpr std::string_view icon{"ic"};
static constexpr std::string_view json_attributes_template{"json_attr_tpl"};
static constexpr std::string_view json_attributes_topic{"json_attr_t"};
static constexpr std::string_view max{"max"};
static constexpr std::string_view min{"min"};
static constexpr std::string_view mode{"mode"};
static constexpr std::string_view name{"name"};
static constexpr std::string_view origin{"o"};
static constexpr std::string_view payload_not_available{"pl_not_avail"};
static constexpr std::string_view payload_off{"pl_off"};
static constexpr std::string_view payload_on{"pl_on"};
static constexpr std::string_view payload_press{"pl_prs"};
static constexpr std::string_view platform{"p"};
static constexpr std::string_view state_class{"stat_cla"};
static constexpr std::string_view state_off{"stat_off"};
static constexpr std::string_view state_on{"stat_on"};
static constexpr std::string_view state_topic{"stat_t"};
static constexpr std::string_view step{"step"};
static constexpr std::string_view suggested_display_precision{"sug_dsp_prc"};
static constexpr std::string_view topic{"t"};
static constexpr std::string_view unique_id{"uniq_id"};
static constexpr std::string_view unit_of_measurement{"unit_of_meas"};
static constexpr std::string_view value_template{"val_tpl"};
}; // namespace ComponentAbbreviations

namespace DeviceAbbreviations
{
static constexpr std::string_view connections{"cns"};
static constexpr std::string_view hw_version{"hw"};
static constexpr std::string_view identifiers{"ids"};
static constexpr std::string_view manufacturer{"mf"};
static constexpr std::string_view model{"mdl"};
static constexpr std::string_view name{ComponentAbbreviations::name};
/**
 * Abbreviated key for the device software version.
 */
static constexpr std::string_view sw_version{"sw"};
}; // namespace DeviceAbbreviations

namespace OriginAbbreviations
{
static constexpr std::string_view name{DeviceAbbreviations::name};
/**
 * Abbreviated discovery key for the origin support URL.
 */
static constexpr std::string_view support_url{"url"};
}; // namespace OriginAbbreviations

#endif // ARDUINO_ARCH_ESP32