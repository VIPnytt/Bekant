#ifdef ARDUINO_ARCH_ESP32

#include "esp/DeviceService.h"

#include "esp/constants.h" // NOLINT(misc-include-cleaner)

#include <WiFi.h>
#include <format>
#include <nvs.h>

/**
 * @brief Initializes the desk controller, hardware interfaces, network services, and MQTT discovery.
 */
void DeviceService::begin()
{
    Serial.begin(115'200UL);
    vTaskDelay(0b1U << 7U);
#ifdef PIN_ADC
    pinMode(PIN_ADC, ANALOG);
#endif // PIN_ADC
#ifdef PIN_LED
    pinMode(PIN_LED, OUTPUT);
#endif // PIN_LED
    pinMode(PIN_MISO, INPUT);
    pinMode(PIN_MOSI, OUTPUT);
#ifdef PIN_OE
    pinMode(PIN_OE, OUTPUT);
#endif // PIN_OE
    pinMode(PIN_RST, OUTPUT_OPEN_DRAIN);
    pinMode(PIN_SCK, OUTPUT);
#ifdef PIN_TPDN
    pinMode(PIN_TPDN, OUTPUT_OPEN_DRAIN);
#endif // PIN_TPDN
#ifdef PIN_TPUP
    pinMode(PIN_TPUP, OUTPUT_OPEN_DRAIN);
#endif // PIN_TPUP
    unsetButtons();
    digitalWrite(PIN_RST, HIGH);
#ifdef PIN_OE
    nvs_handle_t handle{};
    if (nvs_open("bekant", nvs_open_mode_t::NVS_READONLY, &handle) == ESP_OK)
    {
        uint8_t _oe{};
        if (nvs_get_u8(handle, "oe", &_oe) == ESP_OK)
        {
            oe = static_cast<bool>(_oe);
        }
        nvs_close(handle);
    }
    digitalWrite(PIN_OE, oe ? HIGH : LOW);
#endif // PIN_OE
    WiFi.onEvent(&onConnected, arduino_event_id_t::ARDUINO_EVENT_WIFI_STA_CONNECTED);
    WiFi.onEvent(&onDisconnected, arduino_event_id_t::ARDUINO_EVENT_WIFI_STA_DISCONNECTED);
    WiFi.begin(WIFI_SSID, WIFI_KEY);
    WiFi.waitForConnectResult();
    ota.setHostname(HOSTNAME);
#ifdef OTA_KEY
    ota.setPassword(OTA_KEY);
#endif // OTA_KEY
    ota.begin();
    mqtt.onConnect(&onConnect);
    mqtt.onMessage(&onMessage);
    mqtt.onDisconnect(&onDisconnect);
    mqtt.setClientId(HOSTNAME);
    mqtt.setCredentials(MQTT_USER, MQTT_KEY);
    mqtt.setServer(MQTT_HOST, 1883U);
    mqtt.setWill("bekant/" HOSTNAME "/availability",
                 static_cast<uint8_t>(espMqttClientTypes::SubscribeReturncode::QOS2),
                 true,
                 will.data(),
                 will.size() - 1U);
    mqtt.connect();
    desk.begin();
    isp.begin();
    mqttDiscovery();
}

/**
 * @brief Processes service updates, status indicators, connectivity, and desk state publication.
 *
 * Advances OTA, desk, and ISP services, applies pending LED changes, manages status
 * indicators and button states, saves desk state when required, reconnects network
 * services, and publishes desk metadata when connected.
 */
void DeviceService::handle()
{
    ota.handle();
    desk.handle();
    isp.handle();
    if (pending)
    {
        pending = false;
#ifdef PIN_LED
        led.SetPixelColor(0U, color);
        led.Show();
#endif // PIN_LED
        lastMillis = millis();
    }
    else if (color.B != 0U && color.G == 0U && color.R == 0U && millis() - lastMillis > (0b1U << 7U))
    {
        statusWhite();
    }
    else if (color.B == 0U && color.G != 0U && color.R == 0U && millis() - lastMillis > 0b1U << 11U)
    {
        unsetButtons();
        statusWhite();
    }
    else if (color.B == 0U && color.G == 0U && color.R != 0U && millis() - lastMillis > (0b1U << 15U))
    {
        unsetButtons();
        statusNone();
    }
    else if (color.B != 0U && color.G != 0U && color.R != 0U && millis() - lastMillis > (0b1U << 16U))
    {
        statusNone();
        desk.save();
    }
    else if (millis() - lastMillis > 0b1U << 17U)
    {
        if (!WiFi.isConnected())
        {
            WiFi.reconnect();
        }
        else if (!mqtt.connected())
        {
            mqtt.connect();
        }
        else
        {
            JsonDocument doc{};
            desk.metadata(doc);
            transmit(doc);
        }
        lastMillis = millis();
    }
}

void DeviceService::safeMode()
{
    mqtt.publish("bekant/" HOSTNAME "/availability",
                 static_cast<uint8_t>(espMqttClientTypes::SubscribeReturncode::QOS2),
                 true,
                 "");
    mqtt.unsubscribe("bekant/" HOSTNAME "/set");
    Serial1.end();
}

void DeviceService::unsetButtons()
{
#ifdef PIN_TPDN
    digitalWrite(PIN_TPDN, HIGH);
#endif // PIN_TPDN
#ifdef PIN_TPUP
    digitalWrite(PIN_TPUP, HIGH);
#endif // PIN_TPUP
}

/**
 * @brief Publishes the Home Assistant device discovery configuration.
 */
void DeviceService::mqttDiscovery()
{
    JsonDocument doc{};
    desk.onHomeAssistant(doc);
    onHomeAssistant(doc);
    doc[HomeAssistantAbbreviations::availability][HomeAssistantAbbreviations::payload_not_available].set("");
    doc[HomeAssistantAbbreviations::availability][HomeAssistantAbbreviations::topic].set("bekant/" HOSTNAME
                                                                                         "/availability");
    doc[HomeAssistantAbbreviations::device][HomeAssistantDeviceAbbreviations::connections][0U][0U].set("mac");
    doc[HomeAssistantAbbreviations::device][HomeAssistantDeviceAbbreviations::connections][0U][1U].set(
        WiFi.macAddress());
    doc[HomeAssistantAbbreviations::device][HomeAssistantDeviceAbbreviations::hw_version].set(ARDUINO_BOARD);
    doc[HomeAssistantAbbreviations::device][HomeAssistantDeviceAbbreviations::identifiers][0U].set(
        std::format("0x{:x}", ESP.getEfuseMac()));
    doc[HomeAssistantAbbreviations::device][HomeAssistantDeviceAbbreviations::manufacturer].set("IKEA");
    doc[HomeAssistantAbbreviations::device][HomeAssistantDeviceAbbreviations::model].set("BEKANT");
    doc[HomeAssistantAbbreviations::device][HomeAssistantDeviceAbbreviations::name].set(HOSTNAME);
    doc[HomeAssistantAbbreviations::device][HomeAssistantDeviceAbbreviations::sw_version].set("Bekant 1.0.0");
    doc[HomeAssistantAbbreviations::origin][HomeAssistantOriginAbbreviations::name].set("Bekant");
    doc[HomeAssistantAbbreviations::origin][HomeAssistantOriginAbbreviations::support_url].set(
        "https://github.com/VIPnytt/Bekant");
    const size_t length{measureJson(doc)};
    std::vector<uint8_t> payload(length + 1U);
    serializeJson(doc, payload.data(), length + 1U);
    mqtt.publish(std::format("homeassistant/device/0x{:x}/config", ESP.getEfuseMac()).c_str(),
                 static_cast<uint8_t>(espMqttClientTypes::SubscribeReturncode::QOS0),
                 true,
                 payload.data(),
                 length);
}

void DeviceService::statusBlue()
{
    color.B = 0xFFU;
    color.G = 0U;
    color.R = 0U;
    pending = true;
}

void DeviceService::statusGreen()
{
    color.B = 0U;
    color.G = 0xFFU;
    color.R = 0U;
    pending = true;
}

void DeviceService::statusNone()
{
    color.B = 0U;
    color.G = 0U;
    color.R = 0U;
    pending = true;
}

void DeviceService::statusRed()
{
    color.B = 0U;
    color.G = 0U;
    color.R = 0xFFU;
    pending = true;
}

void DeviceService::statusWhite()
{
    color.B = 0xFFU;
    color.G = 0xFFU;
    color.R = 0xFFU;
    pending = true;
}

/**
 * @brief Publishes the current device state.
 *
 * Adds device status and telemetry fields to the JSON document, then publishes it
 * to the MQTT state topic.
 *
 * @param doc JSON document to augment and publish.
 */
void DeviceService::transmit(JsonDocument &doc)
{
#ifdef PIN_OE
    doc["oe"].set(oe);
#endif // PIN_OE
    doc["reset"].set(reset);
    doc["rssi"].set(WiFi.RSSI());
    doc["temperature"].set(temperatureRead());
    doc["unit"].set(ReferenceHeight::heightUnit);
#ifdef PIN_ADC
    doc["voltage"].set(
        static_cast<float>(analogReadMilliVolts(PIN_ADC) * (Voltage::resistanceVcc + Voltage::resistanceGnd)) /
        static_cast<float>(Voltage::resistanceGnd) / 1'000.0F);
#endif // PIN_ADC
    const size_t length{measureJson(doc)};
    std::vector<char> payload(length + 1U);
    serializeJson(doc, payload.data(), length + 1U);
    mqtt.publish("bekant/" HOSTNAME "/state",
                 static_cast<uint8_t>(espMqttClientTypes::SubscribeReturncode::QOS0),
                 false,
                 reinterpret_cast<const uint8_t *>(payload.data()),
                 length);
}

/**
 * @brief Handles a successful MQTT connection.
 *
 * Subscribes to command messages, publishes retained online availability, and
 * sets the device status indicator to white.
 */
void DeviceService::onConnect(bool sessionPresent) // NOLINT(misc-unused-parameters)
{
    ESP_LOGI("MQTT", "connected");
    device.mqtt.subscribe("bekant/" HOSTNAME "/set",
                          static_cast<uint8_t>(espMqttClientTypes::SubscribeReturncode::QOS2));
    device.mqtt.publish("bekant/" HOSTNAME "/availability",
                        static_cast<uint8_t>(espMqttClientTypes::SubscribeReturncode::QOS1),
                        true,
                        "online");
    device.statusWhite();
}

/**
 * @brief Handles notification that the Wi-Fi connection has been established.
 *
 * Sets the device status indicator to white.
 */
void DeviceService::onConnected(arduino_event_id_t event) // NOLINT(misc-unused-parameters)
{
    ESP_LOGI("Wi-Fi", "connected");
    ESP_LOGV("Wi-Fi", "RSSI %d dBm", WiFi.RSSI());
    ESP_LOGI("Wi-Fi", "hostname " HOSTNAME ".local");
    device.statusWhite();
}

/**
 * @brief Handles MQTT disconnection events by setting the device status to red.
 *
 * @param reason Reason for the MQTT disconnection.
 */
void DeviceService::onDisconnect(espMqttClientTypes::DisconnectReason reason)
{
    ESP_LOGD("MQTT", "disconnect reason %s", espMqttClientTypes::disconnectReasonToString(reason));
    device.statusRed();
}

/**
 * @brief Handles Wi-Fi disconnection events by logging the disconnect reason and setting the status indicator to red.
 *
 * @param event Wi-Fi event identifier.
 * @param info Wi-Fi event information containing the disconnect reason.
 */
void DeviceService::onDisconnected(arduino_event_id_t event, // NOLINT(misc-unused-parameters)
                                   arduino_event_info_t info)
{
    ESP_LOGI("Wi-Fi", "disconnected");
    ESP_LOGD("Wi-Fi",
             "disconnect reason %s",
             WiFi.disconnectReasonName(static_cast<wifi_err_reason_t>(info.wifi_sta_disconnected.reason)));
    device.statusRed();
}

/**
 * @brief Processes a complete MQTT message containing a JSON command.
 *
 * @param properties MQTT message properties.
 * @param topic MQTT topic that received the message.
 * @param payload Message payload.
 * @param len Payload length for the current fragment.
 * @param index Offset of the current fragment within the message.
 * @param total Total message length.
 */
void DeviceService::onMessage(const espMqttClientTypes::MessageProperties &properties, // NOLINT(misc-unused-parameters)
                              const char *topic,                                       // NOLINT(misc-unused-parameters)
                              const uint8_t *payload, size_t len, size_t index, size_t total)
{
    if (index == 0U && len == total)
    {
        JsonDocument doc{}; // NOLINT(misc-const-correctness)
        if (deserializeJson(doc, payload, len) == DeserializationError::Code::Ok)
        {
            device.handleRequest(doc.as<JsonObjectConst>());
        }
    }
}

/**
 * @brief Processes commands from a JSON request.
 *
 * Handles desk actions, button controls, height and preset commands, output-enable and reset state changes, and raw
 * transmissions.
 *
 * @param doc JSON object containing the requested commands.
 */
void DeviceService::handleRequest(JsonObjectConst doc)
{
    if (doc["action"].is<std::string_view>())
    {
        const std::string_view action{doc["action"].as<std::string_view>()};
        if (action == "calibrate")
        {
            device.sendTx("c");
        }
        else if (action == "restart")
        {
            device.statusRed();
            mqtt.publish("bekant/" HOSTNAME "/availability",
                         static_cast<uint8_t>(espMqttClientTypes::SubscribeReturncode::QOS0),
                         true,
                         "");
            digitalWrite(PIN_RST, LOW);
            vTaskDelay(0b1U << 7U);
            ESP.restart();
        }
    }
    if (doc["button"]["down"].is<bool>())
    {
        device.setButtonDown(doc["button"]["down"].as<bool>());
    }
    if (doc["button"]["up"].is<bool>())
    {
        device.setButtonUp(doc["button"]["up"].as<bool>());
    }
    if (doc["desk"].is<float>())
    {
        device.sendTx('e', doc["desk"].as<float>());
    }
    if (doc["oe"].is<bool>())
    {
        device.setOutputEnable(doc["oe"].as<bool>());
    }
    if (doc["preset"]["high"].is<bool>() && doc["preset"]["high"].as<bool>())
    {
        device.sendTx("h");
    }
    if (doc["preset"]["high"].is<float>())
    {
        device.sendTx('h', doc["preset"]["high"].as<float>());
    }
    if (doc["preset"]["low"].is<bool>() && doc["preset"]["low"].as<bool>())
    {
        device.sendTx("l");
    }
    if (doc["preset"]["low"].is<float>())
    {
        device.sendTx('l', doc["preset"]["low"].as<float>());
    }
    if (doc["reset"].is<bool>())
    {
        device.setReset(doc["reset"].as<bool>());
    }
    if (doc["tx"].is<std::string_view>())
    {
        device.sendTx(doc["tx"].as<std::string_view>());
    }
}

/**
 * @brief Sends a desk command for the specified user height.
 *
 * @param command Command prefix to send.
 * @param userHeight Target height in user units.
 */
void DeviceService::sendTx(char command, float userHeight)
{
    sendTx(command +
           std::to_string(lroundf(((userHeight - ReferenceHeight::heightLow) *
                                   static_cast<float>(ReferenceHeight::encoderHigh - ReferenceHeight::encoderLow) /
                                   (ReferenceHeight::heightHigh - ReferenceHeight::heightLow)) +
                                  static_cast<float>(ReferenceHeight::encoderLow))));
}

/**
 * @brief Publishes desk metadata and sends a command payload to the desk controller.
 *
 * @param payload Command payload to transmit.
 */
void DeviceService::sendTx(std::string_view payload)
{
    JsonDocument _doc{};
    desk.metadata(_doc);
    _doc["tx"].set(payload);
    transmit(_doc);
    statusRed();
    Serial1.write(payload.data(), payload.size());
    Serial1.write('\n');
}

/**
 * @brief Sets the optional desk down-button control state.
 *
 * Pressing the button sets the status indicator to red.
 *
 * @param state Whether the down button should be active.
 */
void DeviceService::setButtonDown(bool state)
{
#ifdef PIN_TPDN
    if (state)
    {
        statusRed();
    }
    digitalWrite(PIN_TPDN, state ? LOW : HIGH);
#endif // PIN_TPDN
}

/**
 * @brief Sets the optional up button control state.
 *
 * @param state `true` to activate the up button and `false` to release it.
 */
void DeviceService::setButtonUp(bool state)
{
#ifdef PIN_TPUP
    if (state)
    {
        statusRed();
    }
    digitalWrite(PIN_TPUP, state ? LOW : HIGH);
#endif // PIN_TPUP
}

/**
 * @brief Sets the desk's output-enable state.
 *
 * Updates the output-enable hardware, publishes refreshed desk metadata, and
 * persists the changed state when output-enable support is available.
 *
 * @param state Whether output should be enabled.
 */
void DeviceService::setOutputEnable(bool state)
{
#ifdef PIN_OE
    if (state != oe)
    {
        oe = state;
        digitalWrite(PIN_OE, oe ? HIGH : LOW);
        JsonDocument doc{};
        desk.metadata(doc);
        transmit(doc);
        nvs_handle_t handle{};
        if (nvs_open("bekant", nvs_open_mode_t::NVS_READWRITE, &handle) == ESP_OK)
        {
            nvs_set_u8(handle, "oe", static_cast<uint8_t>(oe));
            nvs_commit(handle);
            nvs_close(handle);
        }
    }
#endif // PIN_OE
}

/**
 * @brief Sets the desk reset state and publishes updated metadata when it changes.
 *
 * @param state Whether reset should be asserted.
 */
void DeviceService::setReset(bool state)
{
    if (state != reset)
    {
        reset = state;
        device.statusRed();
        digitalWrite(PIN_RST, reset ? LOW : HIGH);
        JsonDocument doc{};
        desk.metadata(doc);
        transmit(doc);
    }
}

/**
 * @brief Adds Home Assistant MQTT entity discovery configuration to a JSON document.
 *
 * @param doc JSON document to populate with entity configurations.
 */
void DeviceService::onHomeAssistant(JsonDocument &doc)
{
#ifdef PIN_ADC
    {
        JsonObject adc{doc[HomeAssistantAbbreviations::components]["adc"].to<JsonObject>()};
        adc[HomeAssistantAbbreviations::device_class].set("voltage");
        adc[HomeAssistantAbbreviations::enabled_by_default].set(false);
        adc[HomeAssistantAbbreviations::entity_category].set("diagnostic");
        adc[HomeAssistantAbbreviations::expire_after].set(0b1U << 8U);
        adc[HomeAssistantAbbreviations::icon].set("mdi:alpha-v-circle-outline");
        adc[HomeAssistantAbbreviations::name].set("Power supply");
        adc[HomeAssistantAbbreviations::suggested_display_precision].set(1);
        adc[HomeAssistantAbbreviations::platform].set("sensor");
        adc[HomeAssistantAbbreviations::state_class].set("measurement");
        adc[HomeAssistantAbbreviations::state_topic].set("bekant/" HOSTNAME "/state");
        adc[HomeAssistantAbbreviations::unique_id].set("adc");
        adc[HomeAssistantAbbreviations::unit_of_measurement].set("V");
        adc[HomeAssistantAbbreviations::value_template].set("{{value_json.voltage}}");
    }
#endif // PIN_ADC
    {
        JsonObject calibrate{doc[HomeAssistantAbbreviations::components]["calibrate"].to<JsonObject>()};
        calibrate[HomeAssistantAbbreviations::command_template].set(R"({"action":"{{value}}"})");
        calibrate[HomeAssistantAbbreviations::command_topic].set("bekant/" HOSTNAME "/set");
        calibrate[HomeAssistantAbbreviations::entity_category].set("diagnostic");
        calibrate[HomeAssistantAbbreviations::icon].set("mdi:arrow-collapse-down");
        calibrate[HomeAssistantAbbreviations::name].set("Calibrate");
        calibrate[HomeAssistantAbbreviations::payload_press].set("calibrate");
        calibrate[HomeAssistantAbbreviations::platform].set("button");
        calibrate[HomeAssistantAbbreviations::unique_id].set("calibrate");
    }
    {
        JsonObject highRecall{doc[HomeAssistantAbbreviations::components]["recall_high"].to<JsonObject>()};
        highRecall[HomeAssistantAbbreviations::command_template].set(R"({"preset":{"{{value}}":true}})");
        highRecall[HomeAssistantAbbreviations::command_topic].set("bekant/" HOSTNAME "/set");
        highRecall[HomeAssistantAbbreviations::icon].set("mdi:menu-up-outline");
        highRecall[HomeAssistantAbbreviations::json_attributes_template].set(
            R"({"Preset":{{value_json.preset.high}}})");
        highRecall[HomeAssistantAbbreviations::json_attributes_topic].set("bekant/" HOSTNAME "/state");
        highRecall[HomeAssistantAbbreviations::name].set("Preset high");
        highRecall[HomeAssistantAbbreviations::payload_press].set("high");
        highRecall[HomeAssistantAbbreviations::platform].set("button");
        highRecall[HomeAssistantAbbreviations::unique_id].set("recall_high");
    }
    {
        JsonObject highSensor{doc[HomeAssistantAbbreviations::components]["high_sensor"].to<JsonObject>()};
        highSensor[HomeAssistantAbbreviations::device_class].set("distance");
        highSensor[HomeAssistantAbbreviations::icon].set("mdi:menu-up-outline");
        highSensor[HomeAssistantAbbreviations::name].set("Preset high");
        highSensor[HomeAssistantAbbreviations::platform].set("sensor");
        highSensor[HomeAssistantAbbreviations::state_class].set("measurement");
        highSensor[HomeAssistantAbbreviations::state_topic].set("bekant/" HOSTNAME "/state");
        highSensor[HomeAssistantAbbreviations::suggested_display_precision].set(1U);
        highSensor[HomeAssistantAbbreviations::unique_id].set("high_sensor");
        highSensor[HomeAssistantAbbreviations::unit_of_measurement].set(ReferenceHeight::heightUnit);
        highSensor[HomeAssistantAbbreviations::value_template].set(R"({{value_json.preset.high|round(1)}})");
    }
    {
        JsonObject lowRecall{doc[HomeAssistantAbbreviations::components]["recall_low"].to<JsonObject>()};
        lowRecall[HomeAssistantAbbreviations::command_template].set(R"({"preset":{"{{value}}":true}})");
        lowRecall[HomeAssistantAbbreviations::command_topic].set("bekant/" HOSTNAME "/set");
        lowRecall[HomeAssistantAbbreviations::icon].set("mdi:menu-down-outline");
        lowRecall[HomeAssistantAbbreviations::json_attributes_template].set(R"({"Preset":{{value_json.preset.low}}})");
        lowRecall[HomeAssistantAbbreviations::json_attributes_topic].set("bekant/" HOSTNAME "/state");
        lowRecall[HomeAssistantAbbreviations::name].set("Preset low");
        lowRecall[HomeAssistantAbbreviations::payload_press].set("low");
        lowRecall[HomeAssistantAbbreviations::platform].set("button");
        lowRecall[HomeAssistantAbbreviations::unique_id].set("recall_low");
    }
    {
        JsonObject lowSensor{doc[HomeAssistantAbbreviations::components]["low_sensor"].to<JsonObject>()};
        lowSensor[HomeAssistantAbbreviations::device_class].set("distance");
        lowSensor[HomeAssistantAbbreviations::icon].set("mdi:menu-down-outline");
        lowSensor[HomeAssistantAbbreviations::name].set("Preset low");
        lowSensor[HomeAssistantAbbreviations::platform].set("sensor");
        lowSensor[HomeAssistantAbbreviations::state_class].set("measurement");
        lowSensor[HomeAssistantAbbreviations::state_topic].set("bekant/" HOSTNAME "/state");
        lowSensor[HomeAssistantAbbreviations::suggested_display_precision].set(1U);
        lowSensor[HomeAssistantAbbreviations::unique_id].set("low_sensor");
        lowSensor[HomeAssistantAbbreviations::unit_of_measurement].set(ReferenceHeight::heightUnit);
        lowSensor[HomeAssistantAbbreviations::value_template].set(R"({{value_json.preset.low|round(1)}})");
    }
#ifdef PIN_OE
    {
        JsonObject oe{doc[HomeAssistantAbbreviations::components]["oe"].to<JsonObject>()};
        oe[HomeAssistantAbbreviations::command_template].set(R"({"oe":{{value}}})");
        oe[HomeAssistantAbbreviations::command_topic].set("bekant/" HOSTNAME "/set");
        oe[HomeAssistantAbbreviations::entity_category].set("config");
        oe[HomeAssistantAbbreviations::icon].set("mdi:chip");
        oe[HomeAssistantAbbreviations::name].set("Output enable");
        oe[HomeAssistantAbbreviations::payload_off].set("false");
        oe[HomeAssistantAbbreviations::payload_on].set("true");
        oe[HomeAssistantAbbreviations::state_off].set("False");
        oe[HomeAssistantAbbreviations::state_on].set("True");
        oe[HomeAssistantAbbreviations::platform].set("switch");
        oe[HomeAssistantAbbreviations::state_topic].set("bekant/" HOSTNAME "/state");
        oe[HomeAssistantAbbreviations::unique_id].set("oe");
        oe[HomeAssistantAbbreviations::value_template].set("{{value_json.oe}}");
    }
#endif // PIN_OE
    {
        JsonObject reset{doc[HomeAssistantAbbreviations::components]["reset"].to<JsonObject>()};
        reset[HomeAssistantAbbreviations::command_template].set(R"({"reset":{{value}}})");
        reset[HomeAssistantAbbreviations::command_topic].set("bekant/" HOSTNAME "/set");
        reset[HomeAssistantAbbreviations::enabled_by_default].set(false);
        reset[HomeAssistantAbbreviations::entity_category].set("config");
        reset[HomeAssistantAbbreviations::icon].set("mdi:lock-outline");
        reset[HomeAssistantAbbreviations::name].set("Reset");
        reset[HomeAssistantAbbreviations::payload_off].set("false");
        reset[HomeAssistantAbbreviations::payload_on].set("true");
        reset[HomeAssistantAbbreviations::state_off].set("False");
        reset[HomeAssistantAbbreviations::state_on].set("True");
        reset[HomeAssistantAbbreviations::platform].set("switch");
        reset[HomeAssistantAbbreviations::state_topic].set("bekant/" HOSTNAME "/state");
        reset[HomeAssistantAbbreviations::unique_id].set("reset");
        reset[HomeAssistantAbbreviations::value_template].set("{{value_json.reset}}");
    }
    {
        JsonObject restart{doc[HomeAssistantAbbreviations::components]["reboot"].to<JsonObject>()};
        restart[HomeAssistantAbbreviations::command_template].set(R"({"action":"{{value}}"})");
        restart[HomeAssistantAbbreviations::command_topic].set("bekant/" HOSTNAME "/set");
        restart[HomeAssistantAbbreviations::device_class].set("restart");
        restart[HomeAssistantAbbreviations::enabled_by_default].set(false);
        restart[HomeAssistantAbbreviations::entity_category].set("config");
        restart[HomeAssistantAbbreviations::name].set("Reboot");
        restart[HomeAssistantAbbreviations::payload_press].set("restart");
        restart[HomeAssistantAbbreviations::platform].set("button");
        restart[HomeAssistantAbbreviations::unique_id].set("reboot");
    }
    {
        JsonObject rssi{doc[HomeAssistantAbbreviations::components]["rssi"].to<JsonObject>()};
        rssi[HomeAssistantAbbreviations::device_class].set("signal_strength");
        rssi[HomeAssistantAbbreviations::entity_category].set("diagnostic");
        rssi[HomeAssistantAbbreviations::expire_after].set(0b1U << 8U);
        rssi[HomeAssistantAbbreviations::name].set("Wi-Fi signal");
        rssi[HomeAssistantAbbreviations::platform].set("sensor");
        rssi[HomeAssistantAbbreviations::state_class].set("measurement");
        rssi[HomeAssistantAbbreviations::state_topic].set("bekant/" HOSTNAME "/state");
        rssi[HomeAssistantAbbreviations::unique_id].set("rssi");
        rssi[HomeAssistantAbbreviations::unit_of_measurement].set("dBm");
        rssi[HomeAssistantAbbreviations::value_template].set("{{value_json.rssi}}");
    }
    {
        JsonObject temperature{doc[HomeAssistantAbbreviations::components]["temperature"].to<JsonObject>()};
        temperature[HomeAssistantAbbreviations::device_class].set("temperature");
        temperature[HomeAssistantAbbreviations::enabled_by_default].set(false);
        temperature[HomeAssistantAbbreviations::entity_category].set("diagnostic");
        temperature[HomeAssistantAbbreviations::expire_after].set(0b1U << 8U);
        temperature[HomeAssistantAbbreviations::name].set("Temperature");
        temperature[HomeAssistantAbbreviations::platform].set("sensor");
        temperature[HomeAssistantAbbreviations::state_class].set("measurement");
        temperature[HomeAssistantAbbreviations::state_topic].set("bekant/" HOSTNAME "/state");
        temperature[HomeAssistantAbbreviations::unique_id].set("temperature");
        temperature[HomeAssistantAbbreviations::unit_of_measurement].set("°C");
        temperature[HomeAssistantAbbreviations::value_template].set("{{value_json.temperature}}");
    }
#ifdef PIN_TPDN
    {
        JsonObject tpdn{doc[HomeAssistantAbbreviations::components]["tpdn"].to<JsonObject>()};
        tpdn[HomeAssistantAbbreviations::command_template].set(R"({"button":{"down":{{value}}}})");
        tpdn[HomeAssistantAbbreviations::command_topic].set("bekant/" HOSTNAME "/set");
        tpdn[HomeAssistantAbbreviations::enabled_by_default].set(false);
        tpdn[HomeAssistantAbbreviations::entity_category].set("diagnostic");
        tpdn[HomeAssistantAbbreviations::icon].set("mdi:menu-down-outline");
        tpdn[HomeAssistantAbbreviations::name].set("Button down");
        tpdn[HomeAssistantAbbreviations::payload_off].set("false");
        tpdn[HomeAssistantAbbreviations::payload_on].set("true");
        tpdn[HomeAssistantAbbreviations::state_off].set("False");
        tpdn[HomeAssistantAbbreviations::state_on].set("True");
        tpdn[HomeAssistantAbbreviations::platform].set("switch");
        tpdn[HomeAssistantAbbreviations::state_topic].set("bekant/" HOSTNAME "/state");
        tpdn[HomeAssistantAbbreviations::unique_id].set("tpdn");
        tpdn[HomeAssistantAbbreviations::value_template].set("{{value_json.button.down}}");
    }
#endif // PIN_TPDN
#ifdef PIN_TPUP
    {
        JsonObject tpup{doc[HomeAssistantAbbreviations::components]["tpup"].to<JsonObject>()};
        tpup[HomeAssistantAbbreviations::command_template].set(R"({"button":{"up":{{value}}}})");
        tpup[HomeAssistantAbbreviations::command_topic].set("bekant/" HOSTNAME "/set");
        tpup[HomeAssistantAbbreviations::enabled_by_default].set(false);
        tpup[HomeAssistantAbbreviations::entity_category].set("diagnostic");
        tpup[HomeAssistantAbbreviations::icon].set("mdi:menu-up-outline");
        tpup[HomeAssistantAbbreviations::name].set("Button up");
        tpup[HomeAssistantAbbreviations::payload_off].set("false");
        tpup[HomeAssistantAbbreviations::payload_on].set("true");
        tpup[HomeAssistantAbbreviations::state_off].set("False");
        tpup[HomeAssistantAbbreviations::state_on].set("True");
        tpup[HomeAssistantAbbreviations::platform].set("switch");
        tpup[HomeAssistantAbbreviations::state_topic].set("bekant/" HOSTNAME "/state");
        tpup[HomeAssistantAbbreviations::unique_id].set("tpup");
        tpup[HomeAssistantAbbreviations::value_template].set("{{value_json.button.up}}");
    }
#endif // PIN_TPUP
}

/**
 * @brief Returns the singleton device service instance.
 *
 * @return DeviceService& Reference to the shared device service instance.
 */
DeviceService &DeviceService::getInstance()
{
    static DeviceService instance;
    return instance;
}

DeviceService &device{DeviceService::getInstance()}; // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

#endif // ARDUINO_ARCH_ESP32
