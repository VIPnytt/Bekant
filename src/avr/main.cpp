#ifdef ARDUINO_ARCH_AVR

#include "avr/ControllerService.h"

#include <wiring.h>

/**
 * @brief Initializes the desk service.
 */
void setup() { controller.begin(); }

/**
 * @brief Processes the desk service.
 */
void loop() { controller.handle(); }

#endif // ARDUINO_ARCH_AVR
