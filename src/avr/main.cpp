#ifdef ARDUINO_ARCH_AVR

#include "avr/ControllerService.h"

#include <wiring.h>

/**
 * @brief Initializes the controller service.
 */
void setup() { controller.begin(); }

/**
 * @brief Processes the controller.
 */
void loop() { controller.handle(); }

#endif // ARDUINO_ARCH_AVR
