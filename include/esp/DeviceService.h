#pragma once

#ifdef ARDUINO_ARCH_ESP32

#include "esp/DeskService.h"
#include "esp/IspService.h"
#include "secrets.h"

#include <ArduinoJson.h> // NOLINT(misc-include-cleaner)
#include <ArduinoOTA.h>
#include <NeoPixelBus.h>
#include <espMqttClient.h>

class DeviceService
{
private:
    static constexpr std::array<uint8_t, 1U> will{0U};

    unsigned long lastMillis{0U};

#ifdef PIN_OE
    static inline bool accessory{true};
#endif // PIN_OE

    static inline bool pending{false};
    static inline bool reset{false};

    static inline ArduinoOTAClass ArduinoOTA{};

    static inline DeskService Desk{};

    IspService ISP{};

#ifdef PIN_LED
    NeoPixelBus<NeoGrbFeature, NeoWs2812Method> led{1U, PIN_LED};
#endif // PIN_LED

    static inline RgbColor color{0xFFU, 0xFFU, 0xFFU};

    static inline espMqttClient mqtt{};

    void onHomeAssistant(JsonDocument &doc);

    uint16_t encode(float userHeight);

    static void onConnect(bool sessionPresent);
    static void onConnected(arduino_event_id_t event);
    static void onDisconnect(espMqttClientTypes::DisconnectReason reason);
    static void onDisconnected(arduino_event_id_t event, arduino_event_info_t info);
    static void onMessage(const espMqttClientTypes::MessageProperties &properties, const char *topic,
                          const uint8_t *payload, size_t len, size_t index, size_t total);

public:
    static inline NetworkServer avrServer{328U};

    void begin();
    void handle();
    void safeMode();
    void unsetButtons();

    void mqttDiscovery();
    void statusBlue();
    void statusGreen();
    void statusNone();

    static void statusRed();
    static void statusWhite();
    static void transmit(JsonDocument &doc);

    static DeviceService &getInstance();
};

extern DeviceService &Device;

#endif // ARDUINO_ARCH_ESP32