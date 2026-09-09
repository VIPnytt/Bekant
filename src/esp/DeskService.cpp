#ifdef ARDUINO_ARCH_ESP32

#include "esp/DeskService.h"

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
void DeskService::begin()
{
    Serial.begin(115'200UL);
    vTaskDelay(0b1U << 7U);
    ESP_LOGI("Desk", "Bekant %.*s", static_cast<int>(version.size()), version.data());
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
        nvs_get_u16(handle, "8", &encoder8);
        nvs_get_u16(handle, "9", &encoder9);
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
    console.begin();
    wifi.begin();
    ota.begin();
    isp.begin();
    mqtt.begin();
    fetchRelease();
}

/**
 * @brief Advances service processing and publishes updated desk state.
 *
 * Processes connectivity and status services, handles console and MQTT activity when enabled,
 * releases completed drive outputs, persists unsaved state, and publishes pending or periodic
 * state updates.
 */
void DeskService::handle()
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
float DeskService::decode(float encoder)
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
uint16_t DeskService::encode(float height)
{
    return static_cast<uint16_t>(
        lroundf(((height - ReferenceHeight::heightLow) *
                 static_cast<float>(ReferenceHeight::encoderHigh - ReferenceHeight::encoderLow) /
                 (ReferenceHeight::heightHigh - ReferenceHeight::heightLow)) +
                static_cast<float>(ReferenceHeight::encoderLow)));
}

/**
 * @brief Processes commands from a JSON request.
 *
 * Handles calibration, restart, desk positioning, preset updates, drive control,
 * output enable, reset, and tone commands. Position and preset heights outside
 * the configured reference range are ignored.
 *
 * @param doc JSON object containing the commands to process.
 */
void DeskService::request(JsonObjectConst doc)
{
    if (doc["action"].is<std::string_view>())
    {
        const std::string_view action{doc["action"].as<std::string_view>()};
        if (action == "calibrate")
        {
            console.send(ConsoleHandler::Command::CALIBRATE);
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
        desk.setDriveDown(doc["button"]["down"].as<bool>());
    }
    if (doc["button"]["up"].is<bool>())
    {
        desk.setDriveUp(doc["button"]["up"].as<bool>());
    }
    if (doc["desk"].is<float>() && doc["desk"].as<float>() <= ReferenceHeight::heightHigh &&
        doc["desk"].as<float>() >= ReferenceHeight::heightLow)
    {
        console.send(ConsoleHandler::Command::POSITION, encode(doc["desk"].as<float>()));
    }
    if (doc["oe"].is<bool>())
    {
        desk.setOutputEnable(doc["oe"].as<bool>());
    }
    if (doc["preset"]["high"].is<bool>() && doc["preset"]["high"].as<bool>())
    {
        console.send(ConsoleHandler::Command::PRESET_HIGH);
    }
    if (doc["preset"]["high"].is<float>() && doc["preset"]["high"].as<float>() <= ReferenceHeight::heightHigh &&
        doc["preset"]["high"].as<float>() >= ReferenceHeight::heightLow)
    {
        console.send(ConsoleHandler::Command::PRESET_HIGH, encode(doc["preset"]["high"].as<float>()));
    }
    if (doc["preset"]["low"].is<bool>() && doc["preset"]["low"].as<bool>())
    {
        console.send(ConsoleHandler::Command::PRESET_LOW);
    }
    if (doc["preset"]["low"].is<float>() && doc["preset"]["low"].as<float>() <= ReferenceHeight::heightHigh &&
        doc["preset"]["low"].as<float>() >= ReferenceHeight::heightLow)
    {
        console.send(ConsoleHandler::Command::PRESET_LOW, encode(doc["preset"]["low"].as<float>()));
    }
    if (doc["reset"].is<bool>())
    {
        desk.setReset(doc["reset"].as<bool>());
    }
    if (doc["tone"].is<uint16_t>() && doc["tone"].as<uint16_t>() != 0U)
    {
        console.send(ConsoleHandler::Command::TONE, doc["tone"].as<uint16_t>());
    }
}

/**
 * @brief Disables device processing and disconnects serial and MQTT services.
 */
void DeskService::safeMode()
{
    process = false;
    Serial1.end();
    mqtt.disconnect();
}

/**
 * @brief Persists encoder, preset, and output-enable state to non-volatile storage.
 */
void DeskService::save()
{
    nvs_handle_t handle{};
    if (nvs_open("bekant", nvs_open_mode_t::NVS_READWRITE, &handle) == ESP_OK)
    {
        saved = true;
        nvs_set_u16(handle, "8", encoder8);
        nvs_set_u16(handle, "9", encoder9);
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
void DeskService::transmit(JsonDocument &doc)
{
    doc["button"]["down"].set(buttonDown || driveDown.first);
    doc["button"]["up"].set(buttonUp || driveUp.first);
    doc["desk"].set(decode(static_cast<float>(encoder8 + encoder9) / 2.0F));
    doc["encoders"][0U].set(encoder8);
    doc["encoders"][1U].set(encoder9);
    JsonArray errors{doc["errors"].to<JsonArray>()};
    toErrorArray(errors);
    const float leg8{decode(static_cast<float>(encoder8))};
    const float leg9{decode(static_cast<float>(encoder9))};
    doc["legs"][0U].set(leg8);
    doc["legs"][1U].set(leg9);
#ifdef PIN_OE
    doc["oe"].set(enable);
#endif // PIN_OE
    doc["offset"].set(leg8 - leg9);
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
    if (lengthRx != 0U)
    {
        doc["rx"].set(toHex(std::span<const uint8_t>(payloadRx).subspan(0U, lengthRx)));
    }
    doc["states"][0U].set(state8);
    doc["states"][1U].set(state9);
    doc["temperature"].set(temperatureRead());
    if (lengthTx != 0U)
    {
        doc["tx"].set(toHex(std::span<const uint8_t>(payloadTx).subspan(0U, lengthTx)));
    }
    doc["version"]["installed"].set(version);
    if (!versionLatest.empty())
    {
        doc["version"]["latest"].set(versionLatest);
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
void DeskService::setButtonDown(bool state)
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
void DeskService::setButtonUp(bool state)
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
void DeskService::setDriveDown(bool state)
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
void DeskService::setDriveUp(bool state)
{
#ifdef PIN_TPUP
    driveUp.first = state;
    status.setRed();
    digitalWrite(PIN_TPUP, driveUp.first ? LOW : HIGH);
#endif // PIN_TPUP
}

void DeskService::setError8(uint8_t flags)
{
    if (flags != error8)
    {
        error8 = flags;
        pending = true;
    }
    statusRed();
}

void DeskService::setError9(uint8_t flags)
{
    if (flags != error9)
    {
        error9 = flags;
        pending = true;
    }
    statusRed();
}

void DeskService::setErrorInit(uint8_t flags)
{
    if (flags != errorInit)
    {
        errorInit = flags;
        pending = true;
    }
    statusRed();
}

void DeskService::setErrorLin(uint8_t flags)
{
    if (flags != errorLin)
    {
        errorLin = flags;
        pending = true;
    }
    statusRed();
}

void DeskService::setErrorRx(hardwareSerial_error_t flags)
{
    if (flags != errorRx)
    {
        errorRx = flags;
        pending = true;
    }
    statusRed();
}

void DeskService::setErrorTx(uint8_t flags)
{
    if (flags != errorTx)
    {
        errorTx = flags;
        pending = true;
    }
    statusRed();
}

void DeskService::setNode8(uint16_t position, uint8_t state)
{
    if (position != encoder8 && state != state8)
    {
        encoder8 = position;
        state8 = state;
        error8 = 0U;
        saved = false;
        pending = true;
        statusNode();
    }
    else if (position != encoder8)
    {
        encoder8 = position;
        error8 = 0U;
        saved = false;
        pending = true;
        statusNode();
    }
    else if (state != state8)
    {
        state8 = state;
        error8 = 0U;
        pending = true;
        statusNode();
    }
    else if (error8 != 0U)
    {
        error8 = 0U;
        pending = true;
    }
}

void DeskService::setNode9(uint16_t position, uint8_t state)
{
    if (position != encoder9 && state != state9)
    {
        encoder9 = position;
        state9 = state;
        error9 = 0U;
        saved = false;
        pending = true;
        statusNode();
    }
    else if (position != encoder9)
    {
        encoder9 = position;
        error9 = 0U;
        saved = false;
        pending = true;
        statusNode();
    }
    else if (state != state9)
    {
        state9 = state;
        error9 = 0U;
        pending = true;
        statusNode();
    }
    else if (error9 != 0U)
    {
        error9 = 0U;
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
void DeskService::setOutputEnable(bool state)
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
void DeskService::setPresetHigh(uint16_t preset)
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
void DeskService::setPresetLow(uint16_t preset)
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
void DeskService::setReset(bool state) { digitalWrite(PIN_RST, state ? LOW : HIGH); }

/**
 * @brief Stores a newly received serial payload for publication.
 *
 * @param payload Serial payload bytes to store.
 */
void DeskService::setRx(std::span<const uint8_t> payload)
{
    if (lengthRx != payload.size() || !std::equal(payload.begin(), payload.end(), payloadRx.begin()))
    {
        lengthRx = payload.size();
        std::copy(payload.begin(), payload.end(), payloadRx.begin());
        pending = true;
    }
}

/**
 * @brief Updates the stored transmitted serial payload.
 *
 * @param payload Bytes to store as the transmitted payload.
 */
void DeskService::setTx(std::span<const uint8_t> payload)
{
    if (payload.size() <= payloadTx.size() &&
        (lengthTx != payload.size() || !std::equal(payload.begin(), payload.end(), payloadTx.begin())))
    {
        lengthTx = payload.size();
        std::copy(payload.begin(), payload.end(), payloadTx.begin());
        pending = true;
    }
}

/**
 * @brief Sets the status indicator to red.
 */
void DeskService::statusRed() { status.setRed(); }

/**
 * @brief Sets the status indicator to white.
 */
void DeskService::statusWhite() { status.setWhite(); }

/**
 * @brief Selects the status indicator color from motor, button, and drive activity.
 *
 * @details Uses white for idle motor states, green for exclusive manual button activity
 * without drive output activity, and blue for all other states.
 */
void DeskService::statusNode() // NOLINT(readability-make-member-function-const)
{
    if ((state8 == 0U || state8 == 0x25U || state8 == 0x60U) && (state9 == 0U || state9 == 0x25U || state9 == 0x60U))
    {
        status.setWhite(true);
    }
    else if ((buttonDown && !buttonUp && !driveDown.first && !driveUp.first) ||
             (buttonUp && !buttonDown && !driveDown.first && !driveUp.first))
    {
        status.setGreen();
    }
    else
    {
        status.setBlue();
    }
}

/**
 * @brief Converts a byte span to uppercase hexadecimal text.
 *
 * @param payload Bytes to encode.
 * @return std::string Uppercase hexadecimal representation of the bytes.
 */
std::string DeskService::toHex(std::span<const uint8_t> payload)
{
    constexpr std::array<char, 16U> map{'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};
    std::string hex{};
    hex.reserve(payload.size() * 2U);
    for (const uint8_t byte : payload)
    {
        hex += map.at(static_cast<size_t>(byte >> 4U));
        hex += map.at(static_cast<size_t>(byte & 0xFU));
    }
    return hex;
}

void DeskService::toErrorArray(JsonArray &list)
{
    if ((errorInit & 0b1U) != 0U)
    {
        list.add("probe A: no response");
    }
    if ((errorInit & (0b1U << 1U)) != 0U)
    {
        list.add("probe A: checksum mismatch");
    }
    if ((errorInit & (0b1U << 2U)) != 0U)
    {
        list.add("probe B: no response");
    }
    if ((errorInit & (0b1U << 3U)) != 0U)
    {
        list.add("probe B: checksum mismatch");
    }
    if ((error8 & 0b1U) != 0U)
    {
        list.add("node 8: no response");
    }
    if ((error8 & (0b1U << 1U)) != 0U)
    {
        list.add("node 8: checksum mismatch");
    }
    if ((error9 & 0b1U) != 0U)
    {
        list.add("node 9: no response");
    }
    if ((error9 & (0b1U << 1U)) != 0U)
    {
        list.add("node 9: checksum mismatch");
    }
    if ((errorLin & (0b1U << 3U)) != 0U)
    {
        list.add("USART0: data overrun");
    }
    if ((errorLin & (0b1U << 4U)) != 0U)
    {
        list.add("USART0: frame error");
    }
    if ((errorTx & (0b1U << 3U)) != 0U)
    {
        list.add("USART1: data overrun");
    }
    if ((errorTx & (0b1U << 4U)) != 0U)
    {
        list.add("USART1: frame error");
    }
    switch (errorRx)
    {
    case hardwareSerial_error_t::UART_BREAK_ERROR:
        list.add("UART: break");
        break;
    case hardwareSerial_error_t::UART_BUFFER_FULL_ERROR:
        list.add("UART: buffer full");
        break;
    case hardwareSerial_error_t::UART_FIFO_OVF_ERROR:
        list.add("UART: FIFO overflow");
        break;
    case hardwareSerial_error_t::UART_FRAME_ERROR:
        list.add("UART: frame error");
        break;
    }
}

/**
 * @brief Retrieves the latest firmware release version from GitHub.
 *
 * Stores the release tag without a leading `v` and marks the state for
 * publication when the response is valid. Network, HTTP, or JSON parsing
 * failures leave the current release version unchanged.
 */
void DeskService::fetchRelease()
{
    const std::string userAgent{
        std::string{"Bekant/"}.append(version).append(" (ESP32; +https://github.com/VIPnytt/Bekant)")};
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
            ESP_LOGI("Bekant",
                     "Firmware update available: %.*s -> %s",
                     static_cast<int>(version.size()),
                     version.data(),
                     versionLatest.c_str());
            ESP_LOGI("Bekant", "Release notes: https://github.com/VIPnytt/Bekant/releases/v%s", versionLatest.c_str());
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
void DeskService::onInterruptDown()
{
#ifdef PIN_TPDN
    desk.driveDown.second = digitalRead(PIN_TPDN) == LOW;
    if (desk.driveDown.first)
    {
        desk.driveDown.second ? desk.status.setWhite(true) : desk.status.setRed();
    }
    desk.pending = true;
#endif // PIN_TPDN
}

/**
 * @brief Updates the reset state and status indicator from the reset input.
 */
void DeskService::onInterruptReset()
{
    desk.reset = digitalRead(PIN_RST) == LOW;
    if (desk.reset)
    {
        desk.error8 = 0U;
        desk.error9 = 0U;
        desk.errorInit = 0U;
        desk.errorLin = 0U;
        desk.errorRx = hardwareSerial_error_t::UART_NO_ERROR;
        desk.errorTx = 0U;
        desk.lengthRx = 0U;
        desk.lengthTx = 0U;
        desk.status.setNone(true);
    }
    else
    {
        desk.status.setWhite();
    }
    desk.pending = true;
}

/**
 * @brief Updates the upward drive state after a hardware interrupt.
 *
 * Records the active state of the upward drive input, updates the status indicator
 * when upward driving is requested, and marks the device state for publication.
 */
void DeskService::onInterruptUp()
{
#ifdef PIN_TPUP
    desk.driveUp.second = digitalRead(PIN_TPUP) == LOW;
    if (desk.driveUp.first)
    {
        desk.driveUp.second ? desk.status.setWhite(true) : desk.status.setRed();
    }
    desk.pending = true;
#endif // PIN_TPUP
}

/**
 * @brief Returns the singleton device service instance.
 *
 * @return DeskService& Reference to the shared device service instance.
 */
DeskService &DeskService::getInstance()
{
    static DeskService instance;
    return instance;
}

DeskService &desk{DeskService::getInstance()}; // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

#endif // ARDUINO_ARCH_ESP32
