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

    /**
 * Handles in-system programming operations for the device.
 */
IspHandler isp{};

#ifdef PIN_LED
    NeoPixelBus<NeoGrbFeature, NeoWs2812Method> led{1U, PIN_LED};
#endif // PIN_LED

    /**
 * Processes an incoming desk control request.
 * @param doc JSON object containing the request.
 */

/**
 * Processes Home Assistant-related data.
 * @param doc JSON document containing the data.
 */

/**
 * Transmits string data to the desk controller.
 * @param data Data to transmit.
 */

/**
 * Transmits a height command to the desk controller.
 * @param prefix Command prefix.
 * @param userHeight Target user height.
 */

/**
 * Sets the desk's down button state.
 * @param state Whether the button is pressed.
 */

/**
 * Sets the desk's up button state.
 * @param state Whether the button is pressed.
 */

/**
 * Enables or disables device output.
 * @param state Whether output should be enabled.
 */

/**
 * Sets the device reset state.
 * @param state Whether reset should be active.
 */

/**
 * Handles an MQTT connection event.
 * @param sessionPresent Whether the broker has an existing session.
 */

/**
 * Handles an Arduino connectivity event.
 * @param event Event identifier.
 */

/**
 * Handles an MQTT disconnection event.
 * @param reason Reason for the disconnection.
 */

/**
 * Handles an Arduino disconnection event.
 * @param event Event identifier.
 * @param info Event information.
 */

/**
 * Handles an incoming MQTT message.
 * @param properties Message properties.
 * @param topic Message topic.
 * @param payload Message payload.
 * @param len Payload length.
 * @param index Message fragment offset.
 * @param total Total message length.
 */

/**
 * Initializes the device service and its integrations.
 */

/**
 * Processes ongoing device service tasks.
 */

/**
 * Publishes MQTT discovery information.
 */

/**
 * Places the device in safe mode.
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
 * Releases the desk button controls.
 */

/**
 * Transmits a JSON document.
 * @param doc JSON document to transmit.
 */

/**
 * Provides the singleton device service instance.
 * @return The singleton device service instance.
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