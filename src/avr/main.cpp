#ifdef ARDUINO_ARCH_AVR

#include "avr/DeskService.h"

#include <wiring.h>

/**
 * @brief Initializes the desk service and enables the AVR watchdog timer.
 */
void setup() { desk.begin(); }

void loop() { desk.handle(); }

#endif // ARDUINO_ARCH_AVR
