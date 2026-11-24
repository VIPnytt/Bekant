#ifdef ARDUINO_ARCH_ESP32

#include "esp/MqttHandler.h"

#include "esp/DeskService.h"
#include "esp/HomeAssistantHandler.h"
#include "esp/StatusHandler.h"
#include "esp/secrets.h"

#include <WiFi.h>
#include <format>
#include <vector>

/**
 * @brief Initializes the reconnect timer and MQTT client, then publishes the Home Assistant discovery configuration.
 */
void MqttHandler::begin()
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
    if (!client.connected())
    {
        armReconnectTimer();
    }
    discovery();
}

/**
 * @brief Processes MQTT traffic and attempts reconnection when Wi-Fi is available.
 */
void MqttHandler::handle()
{
    client.loop();
    if (tick.exchange(false, std::memory_order_acq_rel) && !client.connected())
    {
        if (WiFi.isConnected())
        {
            client.connect();
        }
        armReconnectTimer();
    }
}

/**
 * @brief Arms the one-shot reconnect alarm if it is not already running.
 */
void MqttHandler::armReconnectTimer()
{
    if (timer != nullptr && !esp_timer_is_active(timer))
    {
        esp_timer_start_once(timer, 0b1ULL << 27U);
    }
}

/**
 * @brief Stops any active reconnect alarm and clears pending reconnect flags.
 */
void MqttHandler::disarmReconnectTimer()
{
    if (timer != nullptr)
    {
        esp_timer_stop(timer);
    }
    tick.store(false, std::memory_order_release);
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
    disarmReconnectTimer();
    armReconnectTimer();
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
void MqttHandler::onConnect(bool sessionPresent)
{
    static_cast<void>(sessionPresent);
    disarmReconnectTimer();
    ESP_LOGI("MQTT", "connected");
    client.subscribe("bekant/" HOSTNAME "/set", static_cast<uint8_t>(espMqttClientTypes::SubscribeReturncode::QOS2));
    client.publish("bekant/" HOSTNAME "/availability",
                   static_cast<uint8_t>(espMqttClientTypes::SubscribeReturncode::QOS1),
                   true,
                   "online");
}

/**
 * @brief Handles MQTT disconnection events and marks the desk status red.
 *
 * @param reason Reason for the MQTT disconnection.
 */
void MqttHandler::onDisconnect(espMqttClientTypes::DisconnectReason reason)
{
    ESP_LOGI("MQTT", "disconnected");
    ESP_LOGD("MQTT", "disconnect reason %s", espMqttClientTypes::disconnectReasonToString(reason));
    StatusHandler::setRed();
    armReconnectTimer();
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
void MqttHandler::onMessage(const espMqttClientTypes::MessageProperties &properties, const char *topic,
                            const uint8_t *payload, size_t len, size_t index, size_t total)
{
    static_cast<void>(properties);
    static_cast<void>(topic);
    JsonDocument doc{};
    if (index == 0U && len == total && deserializeJson(doc, payload, len) == DeserializationError::Code::Ok)
    {
        desk.parse(doc.as<JsonObjectConst>());
    }
}

/**
 * @brief Marks an MQTT reconnect attempt as due when the one-shot timer expires.
 *
 * @param arg Unused timer callback argument.
 */
void MqttHandler::onTimer(void *arg)
{
    static_cast<void>(arg);
    tick.store(true, std::memory_order_release);
}

#endif // ARDUINO_ARCH_ESP32
