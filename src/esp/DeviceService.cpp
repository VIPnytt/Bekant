#ifdef ARDUINO_ARCH_ESP32

#include "esp/DeviceService.h"

#include "esp/constants.h"

#include <WiFi.h> // NOLINT(misc-include-cleaner)
#include <esp_crt_bundle.h>
#include <esp_http_client.h>
#include <format>
#include <nvs.h>

/**
 * @brief Initializes hardware, restores persisted state, attaches input interrupts, and starts device services.
 *
 * Also checks the latest available firmware release.
 */
void DeviceService::begin()
{
    Serial.begin(115'200UL);
    vTaskDelay(0b1U << 7U);
    ESP_LOGI("ESP32", "Bekant %.*s", static_cast<int>(version.size()), version.data());
#ifdef PIN_ADC
    pinMode(PIN_ADC, ANALOG);
#endif // PIN_ADC
#ifdef PIN_LED
    pinMode(PIN_LED, OUTPUT);
#endif // PIN_LED
    pinMode(PIN_MOSI, OUTPUT);
#ifdef PIN_OE
    pinMode(PIN_OE, OUTPUT);
#endif // PIN_OE
    pinMode(PIN_RST, OUTPUT_OPEN_DRAIN);
#ifdef PIN_TPDN
    pinMode(PIN_TPDN, OUTPUT_OPEN_DRAIN);
#endif // PIN_TPDN
#ifdef PIN_TPUP
    pinMode(PIN_TPUP, OUTPUT_OPEN_DRAIN);
#endif // PIN_TPUP
    nvs_handle_t handle{};
    if (nvs_open("bekant", nvs_open_mode_t::NVS_READONLY, &handle) == ESP_OK)
    {
        nvs_get_u16(handle, "a", &encoderA);
        nvs_get_u16(handle, "b", &encoderB);
        nvs_get_u16(handle, "h", &presetHigh);
        nvs_get_u16(handle, "l", &presetLow);
#ifdef PIN_OE
        uint8_t _enable{};
        if (nvs_get_u8(handle, "oe", &_enable) == ESP_OK)
        {
            enable = static_cast<bool>(_enable);
            digitalWrite(PIN_OE, enable ? HIGH : LOW);
        }
#endif // PIN_OE
        nvs_close(handle);
        saved = true;
    }
#ifdef PIN_TPDN
    digitalWrite(PIN_TPDN, HIGH);
#endif // PIN_TPDN
#ifdef PIN_TPUP
    digitalWrite(PIN_TPUP, HIGH);
#endif // PIN_TPUP
    attachInterrupt(PIN_RST, onInterruptReset, CHANGE);
#ifdef PIN_TPDN
    attachInterrupt(PIN_TPDN, onInterruptDown, CHANGE);
#endif // PIN_TPDN
#ifdef PIN_TPUP
    attachInterrupt(PIN_TPUP, onInterruptUp, CHANGE);
#endif // PIN_TPUP
    digitalWrite(PIN_RST, HIGH);
    wifi.begin();
    ota.begin();
    isp.begin();
    console.begin();
    mqtt.begin();
    fetchRelease();
}

/**
 * @brief Advances service processing and publishes updated device state.
 *
 * Processes connectivity, OTA, ISP, and status services. When processing is enabled,
 * handles console and MQTT activity, releases pending drive outputs, saves unsaved
 * state, and publishes state periodically or when an update is pending.
 */
void DeviceService::handle()
{
    wifi.handle();
    ota.handle();
    isp.handle();
    status.handle();
    if (!process)
    {
        status.setNone(true);
        return;
    }
    console.handle();
    mqtt.handle();
    if (pending || millis() - lastMillis > 0b1U << 16U)
    {
#ifdef PIN_TPDN
        if (driveDown.first && !pending)
        {
            digitalWrite(PIN_TPDN, HIGH);
            driveDown.first = false;
        }
#endif // PIN_TPDN
#ifdef PIN_TPUP
        if (driveUp.first && !pending)
        {
            digitalWrite(PIN_TPUP, HIGH);
            driveUp.first = false;
        }
#endif // PIN_TPUP
        if (!saved && !pending)
        {
            save();
        }
        JsonDocument doc{};
        transmit(doc);
        lastMillis = millis();
        pending = false;
    }
}

/**
 * @brief Converts an encoder value to the corresponding physical desk height.
 *
 * @param encoder Encoder value to convert.
 * @return Physical desk height corresponding to the encoder value.
 */
float DeviceService::decode(float encoder)
{
    return ((encoder - static_cast<float>(ReferenceHeight::encoderLow)) *
            (ReferenceHeight::heightHigh - ReferenceHeight::heightLow) /
            static_cast<float>(ReferenceHeight::encoderHigh - ReferenceHeight::encoderLow)) +
           ReferenceHeight::heightLow;
}

/**
 * @brief Converts a physical desk height to its corresponding encoder value.
 *
 * @param height Physical desk height.
 * @return uint16_t Encoder value mapped from the configured height range.
 */
uint16_t DeviceService::encode(float height)
{
    return static_cast<uint16_t>(
        lroundf(((height - ReferenceHeight::heightLow) *
                 static_cast<float>(ReferenceHeight::encoderHigh - ReferenceHeight::encoderLow) /
                 (ReferenceHeight::heightHigh - ReferenceHeight::heightLow)) +
                static_cast<float>(ReferenceHeight::encoderLow)));
}

/**
 * @brief Processes device commands from a JSON request.
 *
 * Handles calibration, restart, desk height, preset, drive-button, output-enable, reset, and raw transmission
 * commands.
 *
 * @param doc JSON object containing the commands to process.
 */
void DeviceService::request(JsonObjectConst doc)
{
    if (doc["action"].is<std::string_view>())
    {
        const std::string_view action{doc["action"].as<std::string_view>()};
        if (action == "calibrate")
        {
            status.setWhite();
            console.send("c");
        }
        else if (action == "restart")
        {
            mqtt.disconnect();
            status.setNone();
            digitalWrite(PIN_RST, LOW);
            vTaskDelay(0b1U << 7U);
            ESP.restart();
        }
    }
    if (doc["button"]["down"].is<bool>())
    {
        device.setDriveDown(doc["button"]["down"].as<bool>());
    }
    if (doc["button"]["up"].is<bool>())
    {
        device.setDriveUp(doc["button"]["up"].as<bool>());
    }
    if (doc["desk"].is<float>() && doc["desk"].as<float>() <= ReferenceHeight::heightHigh &&
        doc["desk"].as<float>() >= ReferenceHeight::heightLow)
    {
        status.setWhite();
        console.send("p" + std::to_string(static_cast<int>(encode(doc["desk"].as<float>()))));
    }
    if (doc["oe"].is<bool>())
    {
        device.setOutputEnable(doc["oe"].as<bool>());
    }
    if (doc["preset"]["high"].is<bool>() && doc["preset"]["high"].as<bool>())
    {
        status.setWhite();
        console.send("h");
    }
    if (doc["preset"]["high"].is<float>() && doc["preset"]["high"].as<float>() <= ReferenceHeight::heightHigh &&
        doc["preset"]["high"].as<float>() >= ReferenceHeight::heightLow)
    {
        status.setWhite();
        console.send("h" + std::to_string(static_cast<int>(encode(doc["preset"]["high"].as<float>()))));
    }
    if (doc["preset"]["low"].is<bool>() && doc["preset"]["low"].as<bool>())
    {
        status.setWhite();
        console.send("l");
    }
    if (doc["preset"]["low"].is<float>() && doc["preset"]["low"].as<float>() <= ReferenceHeight::heightHigh &&
        doc["preset"]["low"].as<float>() >= ReferenceHeight::heightLow)
    {
        status.setWhite();
        console.send("l" + std::to_string(static_cast<int>(encode(doc["preset"]["low"].as<float>()))));
    }
    if (doc["reset"].is<bool>())
    {
        device.setReset(doc["reset"].as<bool>());
    }
    if (doc["tone"].is<uint16_t>() && doc["tone"].as<uint16_t>() != 0U)
    {
        status.setWhite();
        console.send("t" + std::to_string(doc["tone"].as<uint16_t>()));
    }
    if (doc["tx"].is<std::string_view>())
    {
        status.setWhite();
        console.send(doc["tx"].as<std::string_view>());
    }
}

/**
 * @brief Disables device processing and disconnects serial and MQTT services.
 */
void DeviceService::safeMode()
{
    process = false;
    Serial1.end();
    mqtt.disconnect();
}

/**
 * @brief Persists encoder, preset, and output-enable state to non-volatile storage.
 */
void DeviceService::save()
{
    nvs_handle_t handle{};
    if (nvs_open("bekant", nvs_open_mode_t::NVS_READWRITE, &handle) == ESP_OK)
    {
        saved = true;
        nvs_set_u16(handle, "a", encoderA);
        nvs_set_u16(handle, "b", encoderB);
        nvs_set_u16(handle, "h", presetHigh);
        nvs_set_u16(handle, "l", presetLow);
        nvs_set_u8(handle, "oe", static_cast<uint8_t>(enable));
        if (nvs_commit(handle) != ESP_OK)
        {
            saved = false;
        }
        nvs_close(handle);
    }
}

/**
 * @brief Publishes the current device state and telemetry.
 *
 * @param doc JSON document to augment with device state and telemetry before publishing.
 */
void DeviceService::transmit(JsonDocument &doc)
{
    doc["button"]["down"].set(buttonDown || driveDown.first);
    doc["button"]["up"].set(buttonUp || driveUp.first);
    if (encoderA != 0U && encoderB != 0U)
    {
        const float legA{decode(static_cast<float>(encoderA))};
        const float legB{decode(static_cast<float>(encoderB))};
        doc["desk"].set(decode(static_cast<float>(encoderA + encoderB) / 2.0F));
        doc["encoders"][0U].set(encoderA);
        doc["encoders"][1U].set(encoderB);
        doc["legs"][0U].set(legA);
        doc["legs"][1U].set(legB);
        doc["offset"].set(legA - legB);
    }
    else if (encoderA != 0U)
    {
        doc["encoders"][0U].set(encoderA);
        doc["legs"][0U].set(decode(static_cast<float>(encoderA)));
    }
    else if (encoderB != 0U)
    {
        doc["encoders"][1U].set(encoderB);
        doc["legs"][1U].set(decode(static_cast<float>(encoderB)));
    }
    if (!versionAvr.empty())
    {
        doc["firmware"]["avr"].set(versionAvr);
    }
    doc["firmware"]["esp32"].set(version);
    if (!versionLatest.empty())
    {
        doc["firmware"]["latest"].set(versionLatest);
    }
#ifdef PIN_OE
    doc["oe"].set(enable);
#endif // PIN_OE
    if (presetHigh <= ReferenceHeight::encoderHigh && presetHigh >= ReferenceHeight::encoderLow)
    {
        doc["preset"]["high"].set(decode(static_cast<float>(presetHigh)));
    }
    if (presetLow <= ReferenceHeight::encoderHigh && presetLow >= ReferenceHeight::encoderLow)
    {
        doc["preset"]["low"].set(decode(static_cast<float>(presetLow)));
    }
    doc["reset"].set(reset);
    doc["rssi"].set(WiFi.RSSI());
    if (payloadRx.size() != 0U)
    {
        doc["rx"].set(payloadRx);
    }
    doc["temperature"].set(temperatureRead());
    if (payloadTx.size() != 0U)
    {
        doc["tx"].set(payloadTx);
    }
#ifdef PIN_ADC
    doc["voltage"].set(
        static_cast<float>(analogReadMilliVolts(PIN_ADC) * (Voltage::resistanceVcc + Voltage::resistanceGnd)) /
        static_cast<float>(Voltage::resistanceGnd) / 1'000.0F);
#endif // PIN_ADC
    mqtt.transmit(doc);
}

/**
 * @brief Updates the down-button state and requests a state publication.
 *
 * @param state The new down-button state.
 */
void DeviceService::setButtonDown(bool state)
{
    if (state != buttonDown)
    {
        buttonDown = state;
        status.setWhite();
        pending = true;
    }
}

/**
 * @brief Updates the physical up-button state.
 *
 * @param state Whether the up button is pressed.
 */
void DeviceService::setButtonUp(bool state)
{
    if (state != buttonUp)
    {
        buttonUp = state;
        status.setWhite();
        pending = true;
    }
}

/**
 * @brief Sets the optional desk down-drive output state.
 *
 * @param state Whether the down-drive output should be active.
 */
void DeviceService::setDriveDown(bool state)
{
#ifdef PIN_TPDN
    driveDown.first = state;
    status.setRed();
    digitalWrite(PIN_TPDN, state ? LOW : HIGH);
#endif // PIN_TPDN
}

/**
 * @brief Sets the requested state of the optional desk drive-up output.
 *
 * @param state `true` to activate the output; `false` to deactivate it.
 */
void DeviceService::setDriveUp(bool state)
{
#ifdef PIN_TPUP
    driveUp.first = state;
    status.setRed();
    digitalWrite(PIN_TPUP, driveUp.first ? LOW : HIGH);
#endif // PIN_TPUP
}

/**
 * @brief Updates encoder A and marks the device state for publication.
 *
 * Changes to the encoder value mark the persistent state as unsaved and update
 * the status indicator based on the active button or drive controls.
 *
 * @param state New encoder A value.
 */
void DeviceService::setEncoderA(uint16_t state)
{
    if (state != encoderA)
    {
        ((buttonDown && !buttonUp && !driveDown.first && !driveUp.first) ||
         (buttonUp && !buttonDown && !driveDown.first && !driveUp.first))
            ? status.setGreen()
            : status.setBlue();
        encoderA = state;
        saved = false;
        pending = true;
    }
}

/**
 * @brief Updates the secondary encoder value.
 *
 * Marks the device state for persistence and publication when the value changes.
 *
 * @param state New secondary encoder value.
 */
void DeviceService::setEncoderB(uint16_t state)
{
    if (state != encoderB)
    {
        ((buttonDown && !buttonUp && !driveDown.first && !driveUp.first) ||
         (buttonUp && !buttonDown && !driveDown.first && !driveUp.first))
            ? status.setGreen()
            : status.setBlue();
        encoderB = state;
        saved = false;
        pending = true;
    }
}

/**
 * @brief Updates the desk output-enable state.
 *
 * Controls the optional output-enable hardware and marks the state for
 * persistence and publication when output-enable support is configured.
 *
 * @param state Whether the desk output should be enabled.
 */
void DeviceService::setOutputEnable(bool state)
{
#ifdef PIN_OE
    if (state != enable)
    {
        enable = state;
        status.setNone();
        digitalWrite(PIN_OE, enable ? HIGH : LOW);
        saved = false;
        pending = true;
    }
#endif // PIN_OE
}

/**
 * @brief Sets the high preset value and marks the device state for persistence and publication.
 *
 * @param preset High preset value.
 */
void DeviceService::setPresetHigh(uint16_t preset)
{
    if (preset != presetHigh)
    {
        presetHigh = preset;
        saved = false;
        pending = true;
    }
}

/**
 * @brief Sets the lower desk-height preset.
 *
 * @param preset Lower preset value.
 */
void DeviceService::setPresetLow(uint16_t preset)
{
    if (preset != presetLow)
    {
        presetLow = preset;
        saved = false;
        pending = true;
    }
}

/**
 * @brief Sets the desk reset output state.
 *
 * @param state Whether to assert the reset signal.
 */
void DeviceService::setReset(bool state) { digitalWrite(PIN_RST, state ? LOW : HIGH); }

/**
 * @brief Stores the most recently received serial payload.
 *
 * @param payload Received payload to store.
 */
void DeviceService::setRx(std::string_view payload)
{
    if (payload != payloadRx)
    {
        payloadRx = payload;
        pending = true;
    }
}

/**
 * @brief Stores the most recently transmitted serial payload.
 *
 * @param payload Transmitted serial payload.
 */
void DeviceService::setTx(std::string_view payload)
{
    if (payload != payloadTx)
    {
        payloadTx = payload;
        pending = true;
    }
}

/**
 * @brief Stores the AVR firmware version and marks device state for publication.
 *
 * @param avr AVR firmware version.
 */
void DeviceService::setVersion(std::string_view avr)
{
    if (avr != versionAvr)
    {
        versionAvr = avr;
        pending = true;
    }
    if (versionAvr != version)
    {
        ESP_LOGW("AVR",
                 "Firmware update required: %.*s -> %s",
                 static_cast<int>(version.size()),
                 version.data(),
                 versionAvr.c_str());
        ESP_LOGI("AVR",
                 "Release notes: https://github.com/VIPnytt/Bekant/releases/v%.*s",
                 static_cast<int>(version.size()),
                 version.data());
    }
}

/**
 * @brief Sets the status indicator to red.
 */
void DeviceService::statusRed() { status.setRed(); }

/**
 * @brief Checks GitHub for the latest firmware release.
 *
 * Stores the latest release version without its leading `v` and marks the
 * device state for publication when the response is valid. Client, HTTP, and
 * JSON parsing failures leave the release state unchanged.
 */
void DeviceService::fetchRelease()
{
    const std::string userAgent{
        std::string("Bekant/").append(version).append(" (ESP32; +https://github.com/VIPnytt/Bekant)")};
    esp_http_client_config_t config{
        .host{"api.github.com"},
        .port{443},
        .path{"/repos/VIPnytt/Bekant/releases/latest"},
        .user_agent{userAgent.c_str()},
        .method{esp_http_client_method_t::HTTP_METHOD_GET},
        .transport_type{esp_http_client_transport_t::HTTP_TRANSPORT_OVER_SSL},
        .crt_bundle_attach{esp_crt_bundle_attach},
    };
    esp_http_client_handle_t client{esp_http_client_init(&config)};
    if (client == nullptr)
    {
        return;
    }
    esp_http_client_set_header(client, "Accept", "application/vnd.github+json");
    esp_http_client_set_header(client, "X-GitHub-Api-Version", "2026-03-10");
    if (esp_http_client_open(client, 0) != ESP_OK || esp_http_client_fetch_headers(client) < 0 ||
        esp_http_client_get_status_code(client) != 200)
    {
        esp_http_client_cleanup(client);
        return;
    }
    std::vector<char> body{};
    const int64_t length{esp_http_client_get_content_length(client)};
    if (length > 0)
    {
        body.reserve(static_cast<size_t>(length));
    }
    std::array<char, 0b1U << 8U> buffer{};
    while (true)
    {
        const int read{esp_http_client_read(client, buffer.data(), static_cast<int>(buffer.size()))};
        if (read <= 0)
        {
            break;
        }
        body.insert(body.end(), buffer.data(), buffer.data() + read);
    }
    esp_http_client_cleanup(client);
    JsonDocument filter{}; // NOLINT(misc-const-correctness)
    filter["tag_name"].set(true);
    JsonDocument doc{}; // NOLINT(misc-const-correctness)
    if (deserializeJson(doc, body.data(), body.size(), DeserializationOption::Filter(filter)) ==
            DeserializationError::Ok &&
        doc["tag_name"].is<std::string_view>())
    {
        const std::string_view tag{doc["tag_name"].as<std::string_view>()};
        versionLatest = tag.starts_with('v') ? tag.substr(1U) : tag;
        if (versionLatest != version)
        {
            ESP_LOGI("ESP32",
                     "Firmware update available: %.*s -> %s",
                     static_cast<int>(version.size()),
                     version.data(),
                     versionLatest.c_str());
            ESP_LOGI("ESP32", "Release notes: https://github.com/VIPnytt/Bekant/releases/v%s", versionLatest.c_str());
        }
        pending = true;
    }
}

/**
 * @brief Updates the down-drive state from its input pin.
 *
 * Records the physical down-drive state, updates the status indicator for an
 * active down-drive request, and marks the device state for publication.
 */
void DeviceService::onInterruptDown()
{
#ifdef PIN_TPDN
    device.driveDown.second = digitalRead(PIN_TPDN) == LOW;
    if (device.driveDown.first)
    {
        device.driveDown.second ? device.status.setWhite(true) : device.status.setRed();
    }
    device.pending = true;
#endif // PIN_TPDN
}

/**
 * @brief Updates the reset state and status indicator from the reset input.
 */
void DeviceService::onInterruptReset()
{
    device.reset = digitalRead(PIN_RST) == LOW;
    device.reset ? device.status.setNone(true) : device.status.setWhite();
    device.pending = true;
}

/**
 * @brief Updates the upward drive state after a hardware interrupt.
 *
 * Records the active state of the upward drive input, updates the status indicator
 * when upward driving is requested, and marks the device state for publication.
 */
void DeviceService::onInterruptUp()
{
#ifdef PIN_TPUP
    device.driveUp.second = digitalRead(PIN_TPUP) == LOW;
    if (device.driveUp.first)
    {
        device.driveUp.second ? device.status.setWhite(true) : device.status.setRed();
    }
    device.pending = true;
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
