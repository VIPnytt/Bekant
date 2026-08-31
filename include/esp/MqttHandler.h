#pragma once

#ifdef ARDUINO_ARCH_ESP32

#include <ArduinoJson.h> // NOLINT(misc-include-cleaner)
#include <espMqttClient.h>

class MqttHandler
{
private:
    static constexpr std::array<uint8_t, 1U> will{0U};

    unsigned long lastMillis{0U};

    /**
 * Configures MQTT discovery information.
 */

/**
 * Handles a successful MQTT connection.
 *
 * @param sessionPresent Whether the broker resumed an existing session.
 */

/**
 * Handles an MQTT disconnection.
 *
 * @param reason Reason for the disconnection.
 */

/**
 * Processes an incoming MQTT message.
 *
 * @param properties Message metadata.
 * @param topic Message topic.
 * @param payload Message payload.
 * @param len Length of the current payload segment.
 * @param index Offset of the current segment in the complete payload.
 * @param total Length of the complete payload.
 */

/**
 * Initializes MQTT communication.
 */

/**
 * Processes pending MQTT activity.
 */

/**
 * Terminates the MQTT connection.
 */

/**
 * Publishes an ArduinoJson document over MQTT.
 *
 * @param doc Document to publish.
 */
static inline espMqttClient client{espMqttClientTypes::UseInternalTask::NO};

    void discovery();

    static void onConnect(bool sessionPresent);
    static void onDisconnect(espMqttClientTypes::DisconnectReason reason);
    static void onMessage(const espMqttClientTypes::MessageProperties &properties, const char *topic,
                          const uint8_t *payload, size_t len, size_t index, size_t total);

public:
    void begin();
    void handle();

    void disconnect();
    void transmit(JsonDocument &doc);
};

#endif // ARDUINO_ARCH_ESP32
