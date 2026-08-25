#ifdef ARDUINO_ARCH_AVR

#include "avr/DeskService.h"

#include <avr/wdt.h>

void setup()
{
    wdt_enable(WDTO_8S);
    desk.begin();
    wdt_reset();
}

void loop()
{
    desk.handle();
    wdt_reset();
}

#endif // ARDUINO_ARCH_AVR
