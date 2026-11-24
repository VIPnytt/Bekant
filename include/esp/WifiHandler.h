#pragma once

#ifdef ARDUINO_ARCH_ESP32

#include <NetworkEvents.h>
#include <atomic>
#include <esp_timer.h>

class WifiHandler
{
private:
    static inline esp_timer_handle_t timer{nullptr};

    static inline std::atomic<bool> tick{false};

    static void armReconnectTimer();

    static void disarmReconnectTimer();

    static void onConnected(arduino_event_id_t event);

    static void onDisconnected(arduino_event_id_t event, arduino_event_info_t info);

    static void onGotIp(arduino_event_id_t event);

    static void onTimer(void *arg);

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
