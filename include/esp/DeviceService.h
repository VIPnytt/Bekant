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

    /**
 * Processes an incoming JSON request.
 * @param doc JSON request to process.
 */

/**
 * Processes Home Assistant JSON data.
 * @param doc JSON document containing Home Assistant data.
 */

/**
 * Transmits string data.
 * @param data Data to transmit.
 */

/**
 * Transmits a height command for the specified user height.
 * @param prefix Command prefix.
 * @param userHeight User height to transmit.
 */

/**
 * Sets the desk down-button state.
 * @param state Whether the button is pressed.
 */

/**
 * Sets the desk up-button state.
 * @param state Whether the button is pressed.
 */

/**
 * Sets the output-enable state.
 * @param state Whether outputs are enabled.
 */

/**
 * Sets the reset state.
 * @param state Whether reset is active.
 */

/**
 * Handles an MQTT connection event.
 * @param sessionPresent Whether the MQTT session is already present.
 */

/**
 * Handles a network connection event.
 * @param event Network event identifier.
 */

/**
 * Handles an MQTT disconnection event.
 * @param reason Reason for the disconnection.
 */

/**
 * Handles a network disconnection event.
 * @param event Network event identifier.
 * @param info Network event information.
 */

/**
 * Handles an incoming MQTT message.
 * @param properties MQTT message properties.
 * @param topic Message topic.
 * @param payload Message payload.
 * @param len Length of the current payload segment.
 * @param index Offset of the current payload segment.
 * @param total Total payload length.
 */

/**
 * Handles OTA startup.
 */

/**
 * Initializes the device service.
 */

/**
 * Processes ongoing device service activity.
 */

/**
 * Publishes MQTT discovery information.
 */

/**
 * Activates safe-mode behavior.
 */

/**
 * Sets the status indication to blue.
 */

/**
 * Sets the status indication to green.
 */

/**
 * Clears the status indication.
 */

/**
 * Sets the status indication to red.
 */

/**
 * Sets the status indication to white.
 */

/**
 * Releases both desk buttons.
 */

/**
 * Transmits JSON data.
 * @param doc JSON document to transmit.
 */

/**
 * Provides the singleton device service instance.
 * @returns The singleton device service instance.
 */
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