#ifdef ARDUINO_ARCH_ESP32

#include "esp/main.h"

#include "esp/DeviceService.h"

void setup() { device.begin(); }

void loop()
{
    device.handle();
    vTaskDelay(1U);
}

#endif // ARDUINO_ARCH_ESP32
