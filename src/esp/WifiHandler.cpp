#ifdef ARDUINO_ARCH_ESP32

#include "esp/WifiHandler.h"

#include "esp/StatusHandler.h"
#include "esp/secrets.h"

#include <WiFi.h>

/**
 * @brief Configures the Wi-Fi hostname, reconnect timer, and event handlers, then starts Wi-Fi.
 */
void WifiHandler::begin()
{
    if (timer == nullptr)
    {
        const esp_timer_create_args_t args{
            .callback{&onTimer},
            .arg{nullptr},
            .dispatch_method{esp_timer_dispatch_t::ESP_TIMER_TASK},
            .skip_unhandled_events{true},
        };
        esp_timer_create(&args, &timer);
    }

    WiFiClass::setHostname(HOSTNAME);
    WiFi.onEvent(&onConnected, arduino_event_id_t::ARDUINO_EVENT_WIFI_STA_CONNECTED);
    WiFi.onEvent(&onDisconnected, arduino_event_id_t::ARDUINO_EVENT_WIFI_STA_DISCONNECTED);
    WiFi.onEvent(&onGotIp, arduino_event_id_t::ARDUINO_EVENT_WIFI_STA_GOT_IP);
    WiFi.begin(WIFI_SSID, WIFI_KEY);
    WiFi.waitForConnectResult();

    if (!WiFi.isConnected())
    {
        armReconnectTimer();
    }
}

/**
 * @brief Attempts to reconnect to Wi-Fi after a prolonged disconnection.
 */
void WifiHandler::handle()
{
    if (tick.exchange(false, std::memory_order_acq_rel) && !WiFi.isConnected())
    {
        WiFi.reconnect();
        armReconnectTimer();
    }
}

/**
 * @brief Arms the one-shot reconnect alarm if it is not already running.
 */
void WifiHandler::armReconnectTimer()
{
    if (timer != nullptr && !esp_timer_is_active(timer))
    {
        esp_timer_start_once(timer, 0b1ULL << 30U);
    }
}

/**
 * @brief Stops any active reconnect alarm and clears pending reconnect flags.
 */
void WifiHandler::disarmReconnectTimer()
{
    if (timer != nullptr)
    {
        esp_timer_stop(timer);
    }
    tick.store(false, std::memory_order_release);
}

/**
 * @brief Logs that the device connected to Wi-Fi and records the current signal strength.
 *
 * @param event Wi-Fi event identifier.
 */
void WifiHandler::onConnected(arduino_event_id_t event)
{
    static_cast<void>(event);
    ESP_LOGI("Wi-Fi", "connected");
    ESP_LOGV("Wi-Fi", "RSSI %d dBm", WiFi.RSSI());
}

/**
 * @brief Handles a Wi-Fi disconnection event by setting the desk status indicator to red and arming reconnect.
 *
 * @param event Wi-Fi event identifier.
 * @param info Information containing the disconnection reason.
 */
void WifiHandler::onDisconnected(arduino_event_id_t event, arduino_event_info_t info)
{
    static_cast<void>(event);
    static_cast<void>(info);
    ESP_LOGI("Wi-Fi", "disconnected");
    ESP_LOGD("Wi-Fi",
             "disconnect reason %s",
             WiFi.disconnectReasonName(static_cast<wifi_err_reason_t>(info.wifi_sta_disconnected.reason)));
    StatusHandler::setRed();
    armReconnectTimer();
}

/**
 * @brief Disarms the reconnect alarm once an IP address has been obtained.
 *
 * @param event Wi-Fi event identifier.
 */
void WifiHandler::onGotIp(arduino_event_id_t event)
{
    static_cast<void>(event);
    disarmReconnectTimer();
}

/**
 * @brief Marks a Wi-Fi reconnect attempt as due when the one-shot timer expires.
 *
 * @param arg Unused timer callback argument.
 */
void WifiHandler::onTimer(void *arg)
{
    static_cast<void>(arg);
    tick.store(true, std::memory_order_release);
}

#endif // ARDUINO_ARCH_ESP32
