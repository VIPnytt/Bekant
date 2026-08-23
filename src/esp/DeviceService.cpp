#ifdef ARDUINO_ARCH_ESP32

#include "esp/DeviceService.h"

#include "esp/DeskService.h"
#include "esp/IspService.h"
#include "esp/constants.h"
#include "esp/secrets.h"

#include <ESPmDNS.h>
#include <WiFi.h>
#include <format>
#include <nvs.h>

void DeviceService::begin()
{
    Serial.begin(115'200UL);
    vTaskDelay(INT8_MAX);
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
    else if (color.B != 0U && color.G == 0U && color.R == 0U && millis() - lastMillis > INT8_MAX)
    {
        statusWhite();
    }
    else if (color.B == 0U && color.G != 0U && color.R == 0U && millis() - lastMillis > 0b1U << 11U)
    {
        unsetButtons();
        statusWhite();
    }
    else if (color.B == 0U && color.G == 0U && color.R != 0U && millis() - lastMillis > INT16_MAX)
    {
        unsetButtons();
        statusNone();
    }
    else if (color.B != 0U && color.G != 0U && color.R != 0U && millis() - lastMillis > UINT16_MAX)
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

void DeviceService::onConnect(bool sessionPresent)
{
    ESP_LOGD("MQTT", "connected");
    mqtt.subscribe("bekant/" HOSTNAME "/set", static_cast<uint8_t>(espMqttClientTypes::SubscribeReturncode::QOS2));
    mqtt.publish("bekant/" HOSTNAME "/availability",
                 static_cast<uint8_t>(espMqttClientTypes::SubscribeReturncode::QOS1),
                 true,
                 "online");
    statusWhite();
}

void DeviceService::onConnected(arduino_event_id_t event)
{
    ESP_LOGD("Wi-Fi", "Connected");
    ESP_LOGV("Wi-Fi", "RSSI %d dBm", WiFi.RSSI());
    ESP_LOGI("Wi-Fi", "Hostname " HOSTNAME ".local");
    statusWhite();
}

void DeviceService::onDisconnect(espMqttClientTypes::DisconnectReason reason)
{
    ESP_LOGW("MQTT", "%s", espMqttClientTypes::disconnectReasonToString(reason));
    statusRed();
}

void DeviceService::onDisconnected(arduino_event_id_t event, arduino_event_info_t info)
{

    ESP_LOGI("Wi-Fi", "Disconnected");
    ESP_LOGD("Wi-Fi",
             "Disconnect reason %s",
             WiFi.disconnectReasonName(static_cast<wifi_err_reason_t>(info.wifi_sta_disconnected.reason)));
    statusRed();
}

void DeviceService::onHomeAssistant(JsonDocument &doc)
{
#ifdef PIN_OE
    {
        JsonObject accessory{doc[HomeAssistantAbbreviations::components]["accessory"].to<JsonObject>()};
        accessory[HomeAssistantAbbreviations::command_template].set(R"({"accessory":{{value}}})");
        accessory[HomeAssistantAbbreviations::command_topic].set("bekant/" HOSTNAME "/set");
        accessory[HomeAssistantAbbreviations::entity_category].set("config");
        accessory[HomeAssistantAbbreviations::icon].set("mdi:chip");
        accessory[HomeAssistantAbbreviations::name].set("Accessory");
        accessory[HomeAssistantAbbreviations::payload_off].set("false");
        accessory[HomeAssistantAbbreviations::payload_on].set("true");
        accessory[HomeAssistantAbbreviations::state_off].set("False");
        accessory[HomeAssistantAbbreviations::state_on].set("True");
        accessory[HomeAssistantAbbreviations::platform].set("switch");
        accessory[HomeAssistantAbbreviations::state_topic].set("bekant/" HOSTNAME "/state");
        accessory[HomeAssistantAbbreviations::unique_id].set("accessory");
        accessory[HomeAssistantAbbreviations::value_template].set("{{value_json.accessory}}");
    }
#endif // PIN_OE
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
#ifdef PIN_TPDN
    {
        JsonObject down{doc[HomeAssistantAbbreviations::components]["down"].to<JsonObject>()};
        down[HomeAssistantAbbreviations::command_template].set(R"({"button":{"down":{{value}}}})");
        down[HomeAssistantAbbreviations::command_topic].set("bekant/" HOSTNAME "/set");
        down[HomeAssistantAbbreviations::enabled_by_default].set(false);
        down[HomeAssistantAbbreviations::entity_category].set("diagnostic");
        down[HomeAssistantAbbreviations::icon].set("mdi:menu-down-outline");
        down[HomeAssistantAbbreviations::name].set("Button down");
        down[HomeAssistantAbbreviations::payload_off].set("false");
        down[HomeAssistantAbbreviations::payload_on].set("true");
        down[HomeAssistantAbbreviations::state_off].set("False");
        down[HomeAssistantAbbreviations::state_on].set("True");
        down[HomeAssistantAbbreviations::platform].set("switch");
        down[HomeAssistantAbbreviations::state_topic].set("bekant/" HOSTNAME "/state");
        down[HomeAssistantAbbreviations::unique_id].set("down");
        down[HomeAssistantAbbreviations::value_template].set("{{value_json.button.down}}");
    }
#endif // PIN_TPDN
    {
        JsonObject high{doc[HomeAssistantAbbreviations::components]["recall_high"].to<JsonObject>()};
        high[HomeAssistantAbbreviations::command_template].set(R"({"preset":{"{{value}}":true}})");
        high[HomeAssistantAbbreviations::command_topic].set("bekant/" HOSTNAME "/set");
        high[HomeAssistantAbbreviations::icon].set("mdi:menu-up-outline");
        high[HomeAssistantAbbreviations::json_attributes_template].set(R"({"Preset":{{value_json.preset.high}}})");
        high[HomeAssistantAbbreviations::json_attributes_topic].set("bekant/" HOSTNAME "/state");
        high[HomeAssistantAbbreviations::name].set("Preset high");
        high[HomeAssistantAbbreviations::payload_press].set("high");
        high[HomeAssistantAbbreviations::platform].set("button");
        high[HomeAssistantAbbreviations::unique_id].set("recall_high");
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
        JsonObject low{doc[HomeAssistantAbbreviations::components]["recall_low"].to<JsonObject>()};
        low[HomeAssistantAbbreviations::command_template].set(R"({"preset":{"{{value}}":true}})");
        low[HomeAssistantAbbreviations::command_topic].set("bekant/" HOSTNAME "/set");
        low[HomeAssistantAbbreviations::icon].set("mdi:menu-down-outline");
        low[HomeAssistantAbbreviations::json_attributes_template].set(R"({"Preset":{{value_json.preset.low}}})");
        low[HomeAssistantAbbreviations::json_attributes_topic].set("bekant/" HOSTNAME "/state");
        low[HomeAssistantAbbreviations::name].set("Preset low");
        low[HomeAssistantAbbreviations::payload_press].set("low");
        low[HomeAssistantAbbreviations::platform].set("button");
        low[HomeAssistantAbbreviations::unique_id].set("recall_low");
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
        reset[HomeAssistantAbbreviations::name].set("Child lock");
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
        rssi[HomeAssistantAbbreviations::expire_after].set(UINT8_MAX);
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
        temperature[HomeAssistantAbbreviations::expire_after].set(UINT8_MAX);
        temperature[HomeAssistantAbbreviations::name].set("Temperature ESP32");
        temperature[HomeAssistantAbbreviations::platform].set("sensor");
        temperature[HomeAssistantAbbreviations::state_class].set("measurement");
        temperature[HomeAssistantAbbreviations::state_topic].set("bekant/" HOSTNAME "/state");
        temperature[HomeAssistantAbbreviations::unique_id].set("temperature");
        temperature[HomeAssistantAbbreviations::unit_of_measurement].set("°C");
        temperature[HomeAssistantAbbreviations::value_template].set("{{value_json.temperature}}");
    }
#ifdef PIN_TPUP
    {
        JsonObject up{doc[HomeAssistantAbbreviations::components]["up"].to<JsonObject>()};
        up[HomeAssistantAbbreviations::command_template].set(R"({"button":{"up":{{value}}}})");
        up[HomeAssistantAbbreviations::command_topic].set("bekant/" HOSTNAME "/set");
        up[HomeAssistantAbbreviations::enabled_by_default].set(false);
        up[HomeAssistantAbbreviations::entity_category].set("diagnostic");
        up[HomeAssistantAbbreviations::icon].set("mdi:menu-up-outline");
        up[HomeAssistantAbbreviations::name].set("Button up");
        up[HomeAssistantAbbreviations::payload_off].set("false");
        up[HomeAssistantAbbreviations::payload_on].set("true");
        up[HomeAssistantAbbreviations::state_off].set("False");
        up[HomeAssistantAbbreviations::state_on].set("True");
        up[HomeAssistantAbbreviations::platform].set("switch");
        up[HomeAssistantAbbreviations::state_topic].set("bekant/" HOSTNAME "/state");
        up[HomeAssistantAbbreviations::unique_id].set("up");
        up[HomeAssistantAbbreviations::value_template].set("{{value_json.button.up}}");
    }
#endif // PIN_TPUP
#ifdef PIN_ADC
    {
        JsonObject voltage{doc[HomeAssistantAbbreviations::components]["voltage"].to<JsonObject>()};
        voltage[HomeAssistantAbbreviations::device_class].set("voltage");
        voltage[HomeAssistantAbbreviations::enabled_by_default].set(false);
        voltage[HomeAssistantAbbreviations::entity_category].set("diagnostic");
        voltage[HomeAssistantAbbreviations::expire_after].set(INT8_MAX);
        voltage[HomeAssistantAbbreviations::icon].set("mdi:alpha-v-circle-outline");
        voltage[HomeAssistantAbbreviations::name].set("Voltage");
        voltage[HomeAssistantAbbreviations::suggested_display_precision].set(1);
        voltage[HomeAssistantAbbreviations::platform].set("sensor");
        voltage[HomeAssistantAbbreviations::state_class].set("measurement");
        voltage[HomeAssistantAbbreviations::state_topic].set("bekant/" HOSTNAME "/state");
        voltage[HomeAssistantAbbreviations::unique_id].set("voltage");
        voltage[HomeAssistantAbbreviations::unit_of_measurement].set("V");
        voltage[HomeAssistantAbbreviations::value_template].set("{{value_json.voltage}}");
    }
#endif // PIN_ADC
}

void DeviceService::onMessage(const espMqttClientTypes::MessageProperties &properties, const char *topic,
                              const uint8_t *payload, size_t len, size_t index, size_t total)
{
    if (len == total)
    {
        JsonDocument doc{};
        if (deserializeJson(doc, payload, len) == DeserializationError::Code::Ok)
        {
            if (doc["action"].is<std::string_view>())
            {
                const std::string_view action{doc["action"].as<std::string_view>()};
                if (action == "calibrate")
                {
                    JsonDocument _doc{};
                    Desk.metadata(_doc);
                    _doc["tx"].set("c");
                    transmit(_doc);
                    statusRed();
                    Serial1.print("c\n");
                }
                else if (action == "restart")
                {
                    digitalWrite(PIN_RST, LOW);
                    ESP.restart();
                }
            }
#ifdef PIN_OE
            if (doc["accessory"].is<bool>() && doc["accessory"].as<bool>() != accessory)
            {
                accessory = doc["accessory"].as<bool>();
                digitalWrite(PIN_OE, accessory ? HIGH : LOW);
                JsonDocument doc{};
                Desk.metadata(doc);
                transmit(doc);
                nvs_handle_t handle{};
                if (nvs_open("bekant", nvs_open_mode_t::NVS_READWRITE, &handle) == ESP_OK)
                {
                    nvs_set_u8(handle, "accessory", static_cast<uint8_t>(accessory));
                    nvs_commit(handle);
                    nvs_close(handle);
                }
            }
#endif // PIN_OE
#ifdef PIN_TPDN
            if (doc["button"]["down"].is<bool>())
            {
                digitalWrite(PIN_TPDN, doc["button"]["down"].as<bool>() ? LOW : HIGH);
            }
#endif // PIN_TPDN
#ifdef PIN_TPUP
            if (doc["button"]["up"].is<bool>())
            {
                digitalWrite(PIN_TPUP, doc["button"]["up"].as<bool>() ? LOW : HIGH);
            }
#endif // PIN_TPUP
            if (doc["desk"].is<float>())
            {
                const std::string payload{"e" + std::to_string(Device.encode(doc["desk"].as<float>()))};
                JsonDocument _doc{};
                Desk.metadata(_doc);
                _doc["tx"].set(payload);
                transmit(_doc);
                statusRed();
                Serial1.write(payload.data(), payload.size());
                Serial1.write('\n');
            }
            if (doc["preset"]["high"].is<bool>() && doc["preset"]["high"].as<bool>())
            {
                JsonDocument _doc{};
                Desk.metadata(_doc);
                _doc["tx"].set("h");
                transmit(_doc);
                statusRed();
                Serial1.print("h\n");
            }
            if (doc["preset"]["high"].is<float>())
            {
                const std::string payload{"h" + std::to_string(Device.encode(doc["preset"]["high"].as<float>()))};
                JsonDocument _doc{};
                Desk.metadata(_doc);
                _doc["tx"].set(payload);
                transmit(_doc);
                statusRed();
                Serial1.write(payload.data(), payload.size());
                Serial1.write('\n');
            }
            if (doc["preset"]["low"].is<bool>() && doc["preset"]["low"].as<bool>())
            {
                JsonDocument _doc{};
                Desk.metadata(_doc);
                _doc["tx"].set("l");
                transmit(_doc);
                statusRed();
                Serial1.print("l\n");
            }
            if (doc["preset"]["low"].is<float>())
            {
                const std::string payload{"l" + std::to_string(Device.encode(doc["preset"]["low"].as<float>()))};
                JsonDocument _doc{};
                Desk.metadata(_doc);
                _doc["tx"].set(payload);
                transmit(_doc);
                statusRed();
                Serial1.write(payload.data(), payload.size());
                Serial1.write('\n');
            }
            if (doc["reset"].is<bool>())
            {
                reset = doc["reset"].as<bool>();
                DeviceService::statusRed();
                digitalWrite(PIN_RST, reset ? LOW : HIGH);
                JsonDocument doc{};
                Desk.metadata(doc);
                transmit(doc);
            }
            if (doc["tx"].is<std::string_view>())
            {
                JsonDocument _doc{};
                const std::string_view payload{doc["tx"].as<std::string_view>()};
                Desk.metadata(_doc);
                _doc["tx"].set(payload);
                transmit(_doc);
                statusRed();
                Serial1.write(payload.data(), payload.size());
                Serial1.write('\n');
            }
        }
    }
}

uint16_t DeviceService::encode(float userHeight)
{
    return static_cast<uint16_t>(
        lroundf(((userHeight - ReferenceHeight::heightLow) *
                 static_cast<float>(ReferenceHeight::encoderHigh - ReferenceHeight::encoderLow) /
                 (ReferenceHeight::heightHigh - ReferenceHeight::heightLow)) +
                static_cast<float>(ReferenceHeight::encoderLow)));
}

DeviceService &DeviceService::getInstance()
{
    static DeviceService instance;
    return instance;
}

DeviceService &Device{DeviceService::getInstance()};

#endif // ARDUINO_ARCH_ESP32
