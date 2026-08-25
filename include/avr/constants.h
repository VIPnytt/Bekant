#pragma once

#ifdef ARDUINO_ARCH_AVR

#include <core_pins.h>
#include <stdint.h> // NOLINT(hicpp-deprecated-headers,modernize-deprecated-headers)

namespace Encoder
{
static constexpr unsigned int maxDelta{0b1U << 8U};
static constexpr unsigned int maxLimit{0b1U << 13U};
static constexpr unsigned int minLimit{0b1U << 8U};
static constexpr unsigned int targetOffset{137U};
} // namespace Encoder

namespace Pin
{
static constexpr unsigned char buttonDown{PIN_PB1};
static constexpr unsigned char buttonUp{PIN_PB0};
static constexpr unsigned char lin{PIN_PA1};
static constexpr unsigned char tone{PIN_PA7};
} // namespace Pin

#endif // ARDUINO_ARCH_AVR
