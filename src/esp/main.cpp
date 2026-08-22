#ifdef ARDUINO_ARCH_ESP32

#include "esp/main.h"

#include "esp/DeviceService.h"

void setup() { Device.begin(); }

void loop()
{
    Device.handle();
    vTaskDelay(1U);
}

#endif // ARDUINO_ARCH_ESP32
