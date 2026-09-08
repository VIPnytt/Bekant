#ifdef ARDUINO_ARCH_ESP32

#include "esp/main.h"

#include "esp/DeskService.h"

/**
 * @brief Initializes the desk service.
 */
void setup() { desk.begin(); }

/**
 * @brief Processes the desk service and yields for one FreeRTOS task tick.
 */
void loop()
{
    desk.handle();
    vTaskDelay(1U);
}

#endif // ARDUINO_ARCH_ESP32
