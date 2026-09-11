#ifdef __AVR__

#include "avr/ControllerService.h"

#ifdef __AVR_ATtiny841__
#include <wiring.h>
#elif defined(__AVR_ATtiny1624__)
#include <api/Common.h>
#endif // __AVR_ATtiny841__

/**
 * @brief Initializes the controller service.
 */
void setup() { controller.begin(); }

/**
 * @brief Processes the controller.
 */
void loop() { controller.handle(); }

#endif // __AVR__
