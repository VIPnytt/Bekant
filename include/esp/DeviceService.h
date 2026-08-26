#pragma once

#ifdef ARDUINO_ARCH_ESP32

#include "esp/DeskHandler.h"
#include "esp/IspHandler.h"
#include "esp/secrets.h"

#include <ArduinoJson.h> // NOLINT(misc-include-cleaner)
#include <ArduinoOTA.h>
#include <NeoPixelBus.h>
#include <espMqttClient.h>

class DeviceService
{
private:
    static constexpr std::array<uint8_t, 1U> will{0U};

    bool oe{true};
    bool pending{false};
    bool reset{false};

    unsigned long lastMillis{0U};

    ArduinoOTAClass ota;

    DeskHandler desk{};

    espMqttClient mqtt{};

    IspHandler isp{};

#ifdef PIN_LED
    NeoPixelBus<NeoGrbFeature, NeoWs2812Method> led{1U, PIN_LED};
#endif // PIN_LED

    RgbColor color{0xFFU, 0xFFU, 0xFFU};

    void handleRequest(JsonObjectConst doc);
    void onHomeAssistant(JsonDocument &doc);
    void sendTx(std::string_view data);
    void sendTx(char prefix, float userHeight);
    void setButtonDown(bool state);
    void setButtonUp(bool state);
    void setOutputEnable(bool state);
    void setReset(bool state);

    static void onConnect(bool sessionPresent);
    static void onConnected(arduino_event_id_t event);
    static void onDisconnect(espMqttClientTypes::DisconnectReason reason);
    static void onDisconnected(arduino_event_id_t event, arduino_event_info_t info);
    static void onMessage(const espMqttClientTypes::MessageProperties &properties, const char *topic,
                          const uint8_t *payload, size_t len, size_t index, size_t total);
    static void onStart();

public:
    void begin();
    void handle();

    void mqttDiscovery();
    void safeMode();
    void statusBlue();
    void statusGreen();
    void statusNone();
    void statusRed();
    void statusWhite();
    void unsetButtons();
    void transmit(JsonDocument &doc);

    static DeviceService &getInstance();
};

extern DeviceService &device; // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

#endif // ARDUINO_ARCH_ESP32