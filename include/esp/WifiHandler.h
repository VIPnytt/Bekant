#pragma once

#ifdef ARDUINO_ARCH_ESP32

#include <WiFi.h>

class WifiHandler
{
private:
    unsigned long lastMillis{0U};

    static void onConnected(arduino_event_id_t event);
    static void onDisconnected(arduino_event_id_t event, arduino_event_info_t info);

public:
    void begin();
    void handle();
};

#endif // ARDUINO_ARCH_ESP32
