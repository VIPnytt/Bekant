#pragma once

#ifdef ARDUINO_ARCH_ESP32

#include <ArduinoJson.h> // NOLINT(misc-include-cleaner)
#include <espMqttClient.h>

class MqttHandler
{
private:
    static constexpr std::array<uint8_t, 1U> will{0U};

    unsigned long lastMillis{0U};

    static inline espMqttClient client{espMqttClientTypes::UseInternalTask::NO};

    /**
     * Configures MQTT discovery for the device.
     */
    void discovery();

    /**
     * Handles a successful MQTT connection.
     * @param sessionPresent Whether the broker resumed an existing session.
     */
    static void onConnect(bool sessionPresent);

    /**
     * Handles an MQTT disconnection.
     * @param reason Reason for the disconnection.
     */
    static void onDisconnect(espMqttClientTypes::DisconnectReason reason);

    /**
     * Processes an incoming MQTT message.
     * @param properties Message metadata.
     * @param topic Message topic.
     * @param payload Message payload.
     * @param len Number of payload bytes in this fragment.
     * @param index Offset of this fragment within the complete message.
     * @param total Total message payload size.
     */
    static void onMessage(const espMqttClientTypes::MessageProperties &properties, const char *topic,
                          const uint8_t *payload, size_t len, size_t index, size_t total);

public:
    /**
     * Initializes the MQTT handler.
     */
    void begin();

    /**
     * Processes pending MQTT activity.
     */
    void handle();

    /**
     * Terminates the MQTT connection.
     */
    void disconnect();

    /**
     * Publishes an ArduinoJson document over MQTT.
     * @param doc Document containing the message to publish.
     */
    void transmit(JsonDocument &doc);
};

#endif // ARDUINO_ARCH_ESP32
