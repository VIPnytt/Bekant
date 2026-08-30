#ifdef ARDUINO_ARCH_ESP32

#include "esp/WifiHandler.h"

#include "esp/DeviceService.h"
#include "esp/secrets.h"

void WifiHandler::begin()
{
    WiFiClass::setHostname(HOSTNAME);
    WiFi.onEvent(&onConnected, arduino_event_id_t::ARDUINO_EVENT_WIFI_STA_CONNECTED);
    WiFi.onEvent(&onDisconnected, arduino_event_id_t::ARDUINO_EVENT_WIFI_STA_DISCONNECTED);
    WiFi.begin(WIFI_SSID, WIFI_KEY);
    WiFi.waitForConnectResult();
}

void WifiHandler::handle()
{
    if (millis() - lastMillis > (0b1U << 20U) && !WiFi.isConnected())
    {
        WiFi.reconnect();
        lastMillis = millis();
    }
}

void WifiHandler::onConnected(arduino_event_id_t event) // NOLINT(misc-unused-parameters)
{
    ESP_LOGI("Wi-Fi", "connected");
    ESP_LOGV("Wi-Fi", "RSSI %d dBm", WiFi.RSSI());
}

void WifiHandler::onDisconnected(arduino_event_id_t event, // NOLINT(misc-unused-parameters)
                                 arduino_event_info_t info)
{
    ESP_LOGI("Wi-Fi", "disconnected");
    ESP_LOGD("Wi-Fi",
             "disconnect reason %s",
             WiFi.disconnectReasonName(static_cast<wifi_err_reason_t>(info.wifi_sta_disconnected.reason)));
    device.statusRed();
}

#endif // ARDUINO_ARCH_ESP32
