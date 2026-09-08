#pragma once

#ifdef ARDUINO_ARCH_ESP32

#include <NetworkEvents.h>

class WifiHandler
{
private:
    unsigned long lastMillis{0U};

    static void onConnected(arduino_event_id_t event);

    static void onDisconnected(arduino_event_id_t event, arduino_event_info_t info);

public:
    /**
     * Initializes Wi-Fi handling.
     */
    void begin();

    /**
     * Processes ongoing Wi-Fi handling.
     */
    void handle();
};

#endif // ARDUINO_ARCH_ESP32
