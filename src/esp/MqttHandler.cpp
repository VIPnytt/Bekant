#ifdef ARDUINO_ARCH_ESP32

#include "esp/MqttHandler.h"

#include "esp/DeviceService.h"
#include "esp/HomeAssistantHandler.h"
#include "esp/secrets.h"

#include <WiFi.h>
#include <format>

/**
 * @brief Initializes the MQTT client and publishes the Home Assistant discovery configuration.
 */
void MqttHandler::begin()
{
    client.onConnect(&onConnect);
    client.onMessage(&onMessage);
    client.onDisconnect(&onDisconnect);
    client.setClientId(HOSTNAME);
    client.setCredentials(MQTT_USER, MQTT_KEY);
    client.setServer(MQTT_HOST, 1883U);
    client.setWill("bekant/" HOSTNAME "/availability",
                   static_cast<uint8_t>(espMqttClientTypes::SubscribeReturncode::QOS2),
                   true,
                   will.data(),
                   will.size() - 1U);
    client.connect();
    discovery();
}

/**
 * @brief Processes MQTT traffic and attempts reconnection when Wi-Fi is available.
 */
void MqttHandler::handle()
{
    client.loop();
    if (millis() - lastMillis > 0b1U << 16U)
    {
        if (!client.connected() && WiFi.isConnected())
        {
            client.connect();
        }
        lastMillis = millis();
    }
}

/**
 * @brief Publishes an unavailable availability status and disconnects from the MQTT broker.
 */
void MqttHandler::disconnect()
{
    client.publish("bekant/" HOSTNAME "/availability",
                   static_cast<uint8_t>(espMqttClientTypes::SubscribeReturncode::QOS0),
                   true,
                   "");
    client.loop();
    client.disconnect();
    client.loop();
    lastMillis = millis();
}

/**
 * @brief Publishes a JSON document to the device state topic.
 *
 * @param doc JSON document to serialize and publish.
 */
void MqttHandler::transmit(JsonDocument &doc)
{
    const size_t length{measureJson(doc)};
    std::vector<char> payload(length + 1U);
    serializeJson(doc, payload.data(), length + 1U);
    client.publish("bekant/" HOSTNAME "/state",
                   static_cast<uint8_t>(espMqttClientTypes::SubscribeReturncode::QOS0),
                   false,
                   reinterpret_cast<const uint8_t *>(payload.data()),
                   length);
}

/**
 * @brief Publishes the Home Assistant device discovery configuration.
 */
void MqttHandler::discovery()
{
    JsonDocument doc{};
    HomeAssistantHandler hass{HomeAssistantHandler(doc)};
    hass.availability();
    hass.components();
    hass.device();
    hass.origin();
    const size_t length{measureJson(doc)};
    std::vector<uint8_t> payload(length + 1U);
    serializeJson(doc, payload.data(), length + 1U);
    client.publish(std::format("homeassistant/device/0x{:x}/config", ESP.getEfuseMac()).c_str(),
                   static_cast<uint8_t>(espMqttClientTypes::SubscribeReturncode::QOS0),
                   true,
                   payload.data(),
                   length);
}

/**
 * @brief Configures MQTT subscriptions and announces the device as online after connecting.
 */
void MqttHandler::onConnect(bool sessionPresent) // NOLINT(misc-unused-parameters)
{
    ESP_LOGI("MQTT", "connected");
    client.subscribe("bekant/" HOSTNAME "/set", static_cast<uint8_t>(espMqttClientTypes::SubscribeReturncode::QOS2));
    client.publish("bekant/" HOSTNAME "/availability",
                   static_cast<uint8_t>(espMqttClientTypes::SubscribeReturncode::QOS1),
                   true,
                   "online");
}

/**
 * @brief Handles MQTT disconnection events by setting the device status to red.
 *
 * @param reason Reason for the MQTT disconnection.
 */
void MqttHandler::onDisconnect(espMqttClientTypes::DisconnectReason reason)
{
    ESP_LOGI("MQTT", "disconnected");
    ESP_LOGD("MQTT", "disconnect reason %s", espMqttClientTypes::disconnectReasonToString(reason));
    device.statusRed();
}

/**
 * @brief Processes complete MQTT payloads containing valid JSON commands.
 *
 * Fragmented messages and payloads that cannot be parsed as JSON are ignored.
 *
 * @param properties MQTT message properties.
 * @param topic MQTT topic that received the message.
 * @param payload MQTT message payload.
 * @param len Length of the current payload fragment.
 * @param index Offset of the current fragment within the message.
 * @param total Total message length.
 */
void MqttHandler::onMessage(const espMqttClientTypes::MessageProperties &properties, // NOLINT(misc-unused-parameters)
                            const char *topic,                                       // NOLINT(misc-unused-parameters)
                            const uint8_t *payload, size_t len, size_t index, size_t total)
{
    JsonDocument doc{}; // NOLINT(misc-const-correctness)
    if (index == 0U && len == total && deserializeJson(doc, payload, len) == DeserializationError::Code::Ok)
    {
        device.request(doc.as<JsonObjectConst>());
    }
}

#endif // ARDUINO_ARCH_ESP32
