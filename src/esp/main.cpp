#ifdef ARDUINO_ARCH_ESP32

#include "esp/main.h"

#include "esp/DeskService.h"

/**
 * @brief Initializes the device service.
 */
void setup() { desk.begin(); }

/**
 * @brief Processes the device and yields for one task tick.
 */
void loop()
{
    desk.handle();
    vTaskDelay(1U);
}

#endif // ARDUINO_ARCH_ESP32
