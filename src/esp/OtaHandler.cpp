#ifdef ARDUINO_ARCH_ESP32

#include "esp/OtaHandler.h"

#include "esp/DeviceService.h"
#include "esp/secrets.h" // NOLINT(misc-include-cleaner)

void OtaHandler::begin()
{
    ota.setHostname(HOSTNAME);
#ifdef OTA_KEY
    ota.setPassword(OTA_KEY);
#endif // OTA_KEY
    ota.onStart(&onStart);
    ota.begin();
}

void OtaHandler::handle() { ota.handle(); }

/**
 * @brief Enters safe mode when an OTA update starts and flips the output-enable state.
 */
void OtaHandler::onStart() { device.safeMode(); }

#endif // ARDUINO_ARCH_ESP32
