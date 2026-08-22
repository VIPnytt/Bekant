#pragma once

#ifdef ARDUINO_ARCH_AVR

#include <core_pins.h>
#include <inttypes.h>

namespace Pin
{
static constexpr uint8_t down{PIN_PB1};
static constexpr uint8_t lin{PIN_PA1};
static constexpr uint8_t tone{PIN_PA7};
static constexpr uint8_t up{PIN_PB0};
} // namespace Pin

#endif // ARDUINO_ARCH_AVR
