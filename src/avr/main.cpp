#ifdef ARDUINO_ARCH_AVR

#include "avr/DeskService.h"

#include <wiring.h>

/**
 * @brief Initializes the desk service.
 */
void setup() { desk.begin(); }

/**
 * @brief Processes the desk service.
 */
void loop() { desk.handle(); }

#endif // ARDUINO_ARCH_AVR
