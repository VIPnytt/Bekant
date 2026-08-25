#ifdef ARDUINO_ARCH_ESP32

#include "esp/main.h"

#include "esp/DeviceService.h"

/**
 * @brief Initializes the device service.
 */
void setup() { device.begin(); }

/**
 * @brief Processes the device and yields for one task tick.
 */
void loop()
{
    device.handle();
    vTaskDelay(1U);
}

#endif // ARDUINO_ARCH_ESP32
