#pragma once

#ifdef ARDUINO_ARCH_ESP32

#include <cstdint>
#include <string_view>

namespace ReferenceHeight
{
static constexpr uint16_t encoderHigh{6'402U};
static constexpr uint16_t encoderLow{390U};
static constexpr float heightHigh{125.4F}; // 49.37 in
static constexpr float heightLow{66.5F};   // 26.18 in
static constexpr std::string_view heightUnit{"cm"};
} // namespace ReferenceHeight

namespace Voltage
{
static constexpr unsigned int resistanceVcc{100'000U}; // Ω
static constexpr uint16_t resistanceGnd{10'000U};      // Ω
} // namespace Voltage

namespace HomeAssistantAbbreviations
{
static constexpr std::string_view availability{"avty"};
static constexpr std::string_view command_template{"cmd_tpl"};
static constexpr std::string_view command_topic{"cmd_t"};
static constexpr std::string_view components{"cmps"};
static constexpr std::string_view device{"dev"};
static constexpr std::string_view device_class{"dev_cla"};
static constexpr std::string_view enabled_by_default{"en"};
static constexpr std::string_view entity_category{"ent_cat"};
static constexpr std::string_view expire_after{"exp_aft"};
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
}; // namespace HomeAssistantAbbreviations

namespace HomeAssistantDeviceAbbreviations
{
static constexpr std::string_view connections{"cns"};
static constexpr std::string_view hw_version{"hw"};
static constexpr std::string_view identifiers{"ids"};
static constexpr std::string_view manufacturer{"mf"};
static constexpr std::string_view model{"mdl"};
static constexpr std::string_view name{HomeAssistantAbbreviations::name};
static constexpr std::string_view sw_version{"sw"};
}; // namespace HomeAssistantDeviceAbbreviations

namespace HomeAssistantOriginAbbreviations
{
static constexpr std::string_view name{HomeAssistantDeviceAbbreviations::name};
static constexpr std::string_view support_url{"url"};
}; // namespace HomeAssistantOriginAbbreviations

#endif // ARDUINO_ARCH_ESP32
