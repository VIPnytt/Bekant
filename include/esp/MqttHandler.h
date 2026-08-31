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
