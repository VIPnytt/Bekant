#ifdef ARDUINO_ARCH_ESP32

#include "esp/OtaHandler.h"

#include "esp/DeskService.h"
#include "esp/StatusHandler.h"
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
    ota.onError(&onError);
    ota.begin();
}

/**
 * @brief Processes pending OTA update requests.
 */
void OtaHandler::handle() { ota.handle(); }

/**
 * @brief Places the desk in safe mode when an OTA update begins.
 */
void OtaHandler::onStart() { desk.safeMode(); }

/**
 * @brief Marks the status indicator red when an OTA update fails.
 *
 * @param error OTA error reported by ArduinoOTA; all error codes are handled identically.
 */
void OtaHandler::onError(ota_error_t error) // NOLINT(misc-unused-parameters)
{
    StatusHandler::setRed();
}

#endif // ARDUINO_ARCH_ESP32
