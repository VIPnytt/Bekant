#ifdef ARDUINO_ARCH_ESP32

#include "esp/DeviceService.h"

#include "esp/DeskHandler.h"
#include "esp/constants.h"

#include <ESPmDNS.h>
#include <WiFi.h>
#include <format>
#include <nvs.h>

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
        uint8_t _accessory{};
        if (nvs_get_u8(handle, "accessory", &_accessory) == ESP_OK)
        {
            accessory = static_cast<bool>(_accessory);
        }
        nvs_close(handle);
    }
    digitalWrite(PIN_OE, accessory ? HIGH : LOW);
#endif // PIN_OE
    WiFi.onEvent(&onConnected, arduino_event_id_t::ARDUINO_EVENT_WIFI_STA_CONNECTED);
    WiFi.onEvent(&onDisconnected, arduino_event_id_t::ARDUINO_EVENT_WIFI_STA_DISCONNECTED);
    WiFi.begin(WIFI_SSID, WIFI_KEY);
    WiFi.waitForConnectResult();
    ArduinoOTA.setHostname(HOSTNAME);
#ifdef OTA_KEY
    ArduinoOTA.setPassword(OTA_KEY);
#endif // OTA_KEY
    ArduinoOTA.begin();
    avrServer.begin();
    MDNS.addService("avrisp", "tcp", 328U);
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
    Desk.begin();
    ISP.begin();
    mqttDiscovery();
}

void DeviceService::handle()
{
    ArduinoOTA.handle();
    Desk.handle();
    ISP.handle();
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
        Desk.save();
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
            Desk.metadata(doc);
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

void DeviceService::mqttDiscovery()
{
    JsonDocument doc{};
    Desk.onHomeAssistant(doc);
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

void DeviceService::transmit(JsonDocument &doc)
{
#ifdef PIN_OE
    doc["accessory"].set(accessory);
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

void DeviceService::onConnect(bool sessionPresent) // NOLINT(misc-unused-parameters)
{
    ESP_LOGI("MQTT", "connected");
    mqtt.subscribe("bekant/" HOSTNAME "/set", static_cast<uint8_t>(espMqttClientTypes::SubscribeReturncode::QOS2));
    mqtt.publish("bekant/" HOSTNAME "/availability",
                 static_cast<uint8_t>(espMqttClientTypes::SubscribeReturncode::QOS1),
                 true,
                 "online");
    statusWhite();
}

void DeviceService::onConnected(arduino_event_id_t event) // NOLINT(misc-unused-parameters)
{
    ESP_LOGI("Wi-Fi", "connected");
    ESP_LOGV("Wi-Fi", "RSSI %d dBm", WiFi.RSSI());
    ESP_LOGI("Wi-Fi", "hostname " HOSTNAME ".local");
    statusWhite();
}

void DeviceService::onDisconnect(espMqttClientTypes::DisconnectReason reason)
{
    ESP_LOGD("MQTT", "disconnect reason %s", espMqttClientTypes::disconnectReasonToString(reason));
    statusRed();
}

// NOLINTNEXTLINE(misc-unused-parameters)
void DeviceService::onDisconnected(arduino_event_id_t event, arduino_event_info_t info)
{

    ESP_LOGI("Wi-Fi", "disconnected");
    ESP_LOGD("Wi-Fi",
             "disconnect reason %s",
             WiFi.disconnectReasonName(static_cast<wifi_err_reason_t>(info.wifi_sta_disconnected.reason)));
    statusRed();
}

// NOLINTNEXTLINE(misc-unused-parameters)
void DeviceService::onMessage(const espMqttClientTypes::MessageProperties &properties, const char *topic,
                              const uint8_t *payload, size_t len, size_t index, size_t total)
{
    if (index == 0U && len == total)
    {
        JsonDocument doc{}; // NOLINT(misc-const-correctness)
        if (deserializeJson(doc, payload, len) == DeserializationError::Code::Ok)
        {
            Device.handleRequest(doc.as<JsonObjectConst>());
        }
    }
}

void DeviceService::handleRequest(JsonObjectConst doc)
{
    if (doc["action"].is<std::string_view>())
    {
        const std::string_view action{doc["action"].as<std::string_view>()};
        if (action == "calibrate")
        {
            Device.sendTx("c");
        }
        else if (action == "restart")
        {
            DeviceService::statusRed();
            mqtt.disconnect();
            digitalWrite(PIN_RST, LOW);
            ESP.restart();
        }
    }
    if (doc["button"]["down"].is<bool>())
    {
        Device.setButton(false, doc["button"]["down"].as<bool>());
    }
    if (doc["button"]["up"].is<bool>())
    {
        Device.setButton(true, doc["button"]["up"].as<bool>());
    }
    if (doc["desk"].is<float>())
    {
        Device.sendTx('e', doc["desk"].as<float>());
    }
    if (doc["oe"].is<bool>())
    {
        Device.setOutputEnable(doc["oe"].as<bool>());
    }
    if (doc["preset"]["high"].is<bool>() && doc["preset"]["high"].as<bool>())
    {
        Device.sendTx("h");
    }
    if (doc["preset"]["high"].is<float>())
    {
        Device.sendTx('h', doc["preset"]["high"].as<float>());
    }
    if (doc["preset"]["low"].is<bool>() && doc["preset"]["low"].as<bool>())
    {
        Device.sendTx("l");
    }
    if (doc["preset"]["low"].is<float>())
    {
        Device.sendTx('l', doc["preset"]["low"].as<float>());
    }
    if (doc["reset"].is<bool>())
    {
        Device.setReset(doc["reset"].as<bool>());
    }
    if (doc["tx"].is<std::string_view>())
    {
        Device.sendTx(doc["tx"].as<std::string_view>());
    }
}

void DeviceService::sendTx(char command, float userHeight)
{
    sendTx(command +
           std::to_string(lroundf(((userHeight - ReferenceHeight::heightLow) *
                                   static_cast<float>(ReferenceHeight::encoderHigh - ReferenceHeight::encoderLow) /
                                   (ReferenceHeight::heightHigh - ReferenceHeight::heightLow)) +
                                  static_cast<float>(ReferenceHeight::encoderLow))));
}

void DeviceService::sendTx(std::string_view payload)
{
    JsonDocument _doc{};
    Desk.metadata(_doc);
    _doc["tx"].set(payload);
    transmit(_doc);
    statusRed();
    Serial1.write(payload.data(), payload.size());
    Serial1.write('\n');
}

void DeviceService::setButton(bool direction, bool state)
{
    if (state)
    {
        statusRed();
    }
#if defined(PIN_TPDN) && defined(PIN_TPUP)
    digitalWrite(direction ? PIN_TPUP : PIN_TPDN, state ? LOW : HIGH);
#endif // defined(PIN_TPDN) && defined(PIN_TPUP)
}

void DeviceService::setOutputEnable(bool state)
{
#ifdef PIN_OE
    if (state != accessory)
    {
        accessory = state;
        digitalWrite(PIN_OE, accessory ? HIGH : LOW);
        JsonDocument doc{};
        Desk.metadata(doc);
        transmit(doc);
        nvs_handle_t handle{};
        if (nvs_open("bekant", nvs_open_mode_t::NVS_READWRITE, &handle) == ESP_OK)
        {
            nvs_set_u8(handle, "oe", static_cast<uint8_t>(accessory));
            nvs_commit(handle);
            nvs_close(handle);
        }
    }
#endif // PIN_OE
}

void DeviceService::setReset(bool state)
{
    if (state != reset)
    {
        reset = state;
        DeviceService::statusRed();
        digitalWrite(PIN_RST, reset ? LOW : HIGH);
        JsonDocument doc{};
        Desk.metadata(doc);
        transmit(doc);
    }
}

void DeviceService::onHomeAssistant(JsonDocument &doc)
{
#ifdef PIN_OE
    {
        JsonObject accessory{doc[HomeAssistantAbbreviations::components]["oe"].to<JsonObject>()};
        accessory[HomeAssistantAbbreviations::command_template].set(R"({"accessory":{{value}}})");
        accessory[HomeAssistantAbbreviations::command_topic].set("bekant/" HOSTNAME "/set");
        accessory[HomeAssistantAbbreviations::entity_category].set("config");
        accessory[HomeAssistantAbbreviations::icon].set("mdi:chip");
        accessory[HomeAssistantAbbreviations::name].set("Output enable");
        accessory[HomeAssistantAbbreviations::payload_off].set("false");
        accessory[HomeAssistantAbbreviations::payload_on].set("true");
        accessory[HomeAssistantAbbreviations::state_off].set("False");
        accessory[HomeAssistantAbbreviations::state_on].set("True");
        accessory[HomeAssistantAbbreviations::platform].set("switch");
        accessory[HomeAssistantAbbreviations::state_topic].set("bekant/" HOSTNAME "/state");
        accessory[HomeAssistantAbbreviations::unique_id].set("oe");
        accessory[HomeAssistantAbbreviations::value_template].set("{{value_json.accessory}}");
    }
#endif // PIN_OE
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

DeviceService &DeviceService::getInstance()
{
    static DeviceService instance;
    return instance;
}

DeviceService &Device{DeviceService::getInstance()};

#endif // ARDUINO_ARCH_ESP32
