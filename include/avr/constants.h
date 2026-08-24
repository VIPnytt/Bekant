#pragma once

#ifdef ARDUINO_ARCH_AVR

#include <core_pins.h>
#include <stdint.h> // NOLINT(hicpp-deprecated-headers,modernize-deprecated-headers)

namespace Pin
{
static constexpr uint8_t buttonDown{PIN_PB1};
static constexpr uint8_t buttonUp{PIN_PB0};
static constexpr uint8_t lin{PIN_PA1};
static constexpr uint8_t tone{PIN_PA7};
} // namespace Pin

#endif // ARDUINO_ARCH_AVR
