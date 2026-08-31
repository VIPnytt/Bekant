#ifdef ARDUINO_ARCH_ESP32

#include "esp/OtaHandler.h"

#include "esp/DeviceService.h"
#include "esp/secrets.h" // NOLINT(misc-include-cleaner)

/**
 * @brief Configures and starts over-the-air update handling for the device.
 */
void OtaHandler::begin()
{
    ota.setHostname(HOSTNAME);
#ifdef OTA_KEY
    ota.setPassword(OTA_KEY);
#endif // OTA_KEY
    ota.onStart(&onStart);
    ota.begin();
}

/**
 * @brief Processes pending OTA update requests.
 */
void OtaHandler::handle() { ota.handle(); }

/**
 * @brief Places the device in safe mode when an OTA update begins.
 */
void OtaHandler::onStart() { device.safeMode(); }

#endif // ARDUINO_ARCH_ESP32
