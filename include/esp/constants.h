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

#endif // ARDUINO_ARCH_ESP32
