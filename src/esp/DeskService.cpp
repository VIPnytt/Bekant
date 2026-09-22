#ifdef ARDUINO_ARCH_ESP32

#include "esp/DeskService.h"

#include "esp/constants.h"
#include "esp/secrets.h"

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
    pinMode(PIN_MOSI, OUTPUT);
#ifdef PIN_OE
    pinMode(PIN_OE, OUTPUT);
#endif // PIN_OE
    pinMode(PIN_RST, OUTPUT_OPEN_DRAIN);
#ifdef PIN_OE
    nvs_handle_t handle{};
    if (nvs_open("bekant", nvs_open_mode_t::NVS_READONLY, &handle) == ESP_OK)
    {
        uint8_t _enable{};
        if (nvs_get_u8(handle, "oe", &_enable) == ESP_OK)
        {
            enable = static_cast<bool>(_enable);
            digitalWrite(PIN_OE, enable ? HIGH : LOW);
        }
        nvs_close(handle);
    }
#endif // PIN_OE
    button.begin();
    leg.begin();
    preset.begin();
    attachInterrupt(PIN_RST, onReset, CHANGE);
    digitalWrite(PIN_RST, HIGH);
    status.begin();
    console.begin();
    tone.begin();
    wifi.begin();
    ota.begin();
    isp.begin();
    mqtt.begin();
    getRelease();
}

/**
 * @brief Advances service processing and publishes updated desk state.
 *
 * Processes connectivity and status services and handles console and MQTT activity when enabled.
 * Periodic updates release completed button simulations and persist pending output-enable changes;
 * state changes are published immediately.
 */
void DeskService::handle()
{
    wifi.handle();
    ota.handle();
    isp.handle();
    status.handle();
    if (!process)
    {
        StatusHandler::setNone(true);
        return;
    }
    console.handle();
    leg.handle();
    preset.handle();
    tone.handle();
    mqtt.handle();
    if (pending || millis() - lastMillis > 0b1U << 16U)
    {
        if (!pending)
        {
            button.resetSimulation();
            if (!saved)
            {
                save();
            }
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
 * @brief Retrieves the latest firmware release version from GitHub.
 *
 * Stores the release tag without a leading `v` and marks the state for
 * publication when the response is valid. Network, HTTP, or JSON parsing
 * failures leave the current release version unchanged.
 */
void DeskService::getRelease()
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
 * @brief Updates the reset state and status indicator from the reset input.
 *
 * Clears recorded communication errors, the AVR reset cause, and captured serial payloads from subsequent publications
 * while reset is asserted.
 */
void DeskService::onReset()
{
    desk.reset = digitalRead(PIN_RST) == LOW;
    if (desk.reset)
    {
        desk.lengthRx = 0U;
        desk.lengthTx = 0U;
        desk.issue.clear();
        StatusHandler::setNone(true);
    }
    else
    {
        StatusHandler::setWhite();
    }
    desk.pending = true;
}

/**
 * @brief Applies a received console frame to the corresponding device state.
 *
 * Invalid command and payload-length combinations set the device status to red.
 *
 * @param state Frame state decoded from the header.
 * @param payload Complete frame, including its header byte.
 */
void DeskService::parse(ConsoleHandler::State state, std::span<const uint8_t> payload)
{
    setRx(payload);
    if (state == ConsoleHandler::State::BUTTONS && payload.size() == 2U)
    {
        button.setStates(payload[1U]);
    }
    else if (state == ConsoleHandler::State::CONSOLE && payload.size() == 2U)
    {
        issue.setConsoleTx(payload[1U]);
    }
    else if (state == ConsoleHandler::State::INITIALIZATION && payload.size() == 2U)
    {
        issue.setInitialization(payload[1U]);
    }
    else if (state == ConsoleHandler::State::LIN && payload.size() == 2U)
    {
        issue.setLegsRx(payload[1U]);
    }
    else if (state == ConsoleHandler::State::NODE8 && payload.size() == 2U)
    {
        issue.setNode8(payload[1U]);
    }
    else if (state == ConsoleHandler::State::NODE8 && payload.size() == 4U)
    {
        leg.setNode8(static_cast<uint16_t>(static_cast<unsigned int>(payload[1U]) |
                                           static_cast<unsigned int>(payload[2U]) << 8U),
                     payload[3U]);
        issue.setNode8(0U);
    }
    else if (state == ConsoleHandler::State::NODE9 && payload.size() == 2U)
    {
        issue.setNode9(payload[1U]);
    }
    else if (state == ConsoleHandler::State::NODE9 && payload.size() == 4U)
    {
        leg.setNode9(static_cast<uint16_t>(static_cast<unsigned int>(payload[1U]) |
                                           static_cast<unsigned int>(payload[2U]) << 8U),
                     payload[3U]);
        issue.setNode9(0U);
    }
    else if (state == ConsoleHandler::State::PRESET_HIGH && payload.size() == 3U)
    {
        preset.setHigh(static_cast<uint16_t>(static_cast<unsigned int>(payload[1U]) |
                                             static_cast<unsigned int>(payload[2U]) << 8U));
    }
    else if (state == ConsoleHandler::State::PRESET_LOW && payload.size() == 3U)
    {
        preset.setLow(static_cast<uint16_t>(static_cast<unsigned int>(payload[1U]) |
                                            static_cast<unsigned int>(payload[2U]) << 8U));
    }
    else if (state == ConsoleHandler::State::RESET_REASON && payload.size() == 2U)
    {
        issue.setResetReason(payload[1U]);
    }
    else if (state == ConsoleHandler::State::VERSION && payload.size() == 2U)
    {
        issue.setVersion(payload[1U]);
    }
    else
    {
        StatusHandler::setRed();
    }
}

/**
 * @brief Processes commands from a JSON request.
 *
 * Handles recalibration, restart, desk positioning, preset updates, optional
 * down/up output simulation, output enable, reset, and tone commands. Position
 * and preset heights outside the configured reference range are ignored.
 * Tone objects reuse the current setting for duration or frequency values that
 * are missing, invalid, or zero.
 *
 * @param doc JSON object containing the commands to process.
 */
void DeskService::parse(JsonObjectConst doc)
{
    if (doc["action"].is<std::string_view>())
    {
        parseAction(doc["action"].as<std::string_view>());
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
        preset.setHigh();
    }
    else if (doc["preset"]["high"].is<float>())
    {
        preset.setHigh(doc["preset"]["high"].as<float>());
    }
    if (doc["preset"]["low"].is<bool>() && doc["preset"]["low"].as<bool>())
    {
        preset.setLow();
    }
    else if (doc["preset"]["low"].is<float>())
    {
        preset.setLow(doc["preset"]["low"].as<float>());
    }
    if (doc["reset"].is<bool>())
    {
        desk.setReset(doc["reset"].as<bool>());
    }
    if (doc["simulate"]["down"].is<bool>())
    {
        button.setSimulateDown(doc["simulate"]["down"].as<bool>());
    }
    if (doc["simulate"]["up"].is<bool>())
    {
        button.setSimulateUp(doc["simulate"]["up"].as<bool>());
    }
    if (doc["tone"].is<JsonObjectConst>())
    {
        tone.parse(doc["tone"].as<JsonObjectConst>());
    }
}

/**
 * @brief Applies a named maintenance action.
 *
 * Supports recalibrating the leg encoder sensors and restarting the ESP32 after placing the AVR controller in reset.
 * Unsupported action names are ignored.
 *
 * @param action Action name from the JSON request; "recalibrate" and "restart" are supported.
 */
void DeskService::parseAction(std::string_view action)
{
    if (action == "power")
    {
        mqtt.disconnect();
        StatusHandler::setNone();
#ifdef PIN_OE
        gpio_hold_en(static_cast<gpio_num_t>(PIN_OE));
#if SOC_GPIO_SUPPORT_HOLD_IO_IN_DSLP && !SOC_GPIO_SUPPORT_HOLD_SINGLE_IO_IN_DSLP
        gpio_deep_sleep_hold_en();
#endif // SOC_GPIO_SUPPORT_HOLD_IO_IN_DSLP && !SOC_GPIO_SUPPORT_HOLD_SINGLE_IO_IN_DSLP
#endif // PIN_OE
        vTaskDelay(1U);
        esp_deep_sleep_start();
    }
    if (action == "recalibrate")
    {
        console.send(ConsoleHandler::Command::RECALIBRATE);
    }
    else if (action == "restart")
    {
        mqtt.disconnect();
        StatusHandler::setNone();
        digitalWrite(PIN_RST, LOW);
        vTaskDelay(1U);
        ESP.restart();
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
 * @brief Persists the output-enable state to non-volatile storage.
 */
void DeskService::save()
{
    nvs_handle_t handle{};
    if (nvs_open("bekant", nvs_open_mode_t::NVS_READWRITE, &handle) == ESP_OK)
    {
        saved = true;
        nvs_set_u8(handle, "oe", static_cast<uint8_t>(enable)); // NOLINT(readability-implicit-bool-conversion)
        if (nvs_commit(handle) != ESP_OK)
        {
            saved = false;
        }
        nvs_close(handle);
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
        StatusHandler::setNone();
#if SOC_GPIO_SUPPORT_HOLD_IO_IN_DSLP && !SOC_GPIO_SUPPORT_HOLD_SINGLE_IO_IN_DSLP
        gpio_deep_sleep_hold_dis();
#endif // SOC_GPIO_SUPPORT_HOLD_IO_IN_DSLP && !SOC_GPIO_SUPPORT_HOLD_SINGLE_IO_IN_DSLP
        gpio_hold_dis(static_cast<gpio_num_t>(PIN_OE));
        digitalWrite(PIN_OE, enable ? HIGH : LOW);
        gpio_hold_en(static_cast<gpio_num_t>(PIN_OE));
#if SOC_GPIO_SUPPORT_HOLD_IO_IN_DSLP && !SOC_GPIO_SUPPORT_HOLD_SINGLE_IO_IN_DSLP
        gpio_deep_sleep_hold_en();
#endif // SOC_GPIO_SUPPORT_HOLD_IO_IN_DSLP && !SOC_GPIO_SUPPORT_HOLD_SINGLE_IO_IN_DSLP
        saved = false;
        pending = true;
    }
#endif // PIN_OE
}

/**
 * @brief Requests device-state publication on the next service cycle.
 */
void DeskService::setPending() { pending = true; }

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
 * @brief Selects the status color from the current leg and button states.
 *
 * Uses white when both legs report idle; otherwise derives green or blue from button activity.
 */
void DeskService::setStatus() { leg.getIdle() ? StatusHandler::setWhite(true) : button.setStatus(); }

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
 * @brief Converts a byte span to uppercase hexadecimal text.
 *
 * @param payload Bytes to encode.
 * @return std::string Uppercase hexadecimal representation of the bytes.
 */
std::string DeskService::toHex(std::span<const uint8_t> payload)
{
    std::string hex{};
    hex.reserve(payload.size() * 2U);
    for (const uint8_t byte : payload)
    {
        const uint8_t high{static_cast<uint8_t>(byte >> 4U)};
        const uint8_t low{static_cast<uint8_t>(byte & 0xFU)};
        hex += static_cast<char>(high < 10U ? '0' + high : 'A' + high - 10U);
        hex += static_cast<char>(low < 10U ? '0' + low : 'A' + low - 10U);
    }
    return hex;
}

/**
 * @brief Publishes the current device state and telemetry.
 *
 * @param doc JSON document to augment with device state and telemetry before publishing.
 */
void DeskService::transmit(JsonDocument &doc)
{
    doc["button"]["3"].set(button.getState3());
    doc["button"]["4"].set(button.getState4());
    doc["button"]["down"].set(button.getDown());
    doc["button"]["up"].set(button.getUp());
    const std::pair<uint16_t, uint16_t> encoders{leg.getEncoders()};
    doc["desk"].set(decode(static_cast<float>(encoders.first + encoders.second) / 2.0F));
    doc["encoders"][0U].set(encoders.first);
    doc["encoders"][1U].set(encoders.second);
    JsonArray issues{doc["issues"].to<JsonArray>()};
    issue.getIssues(issues);
    const std::pair<float, float> legs{leg.getLegs()};
    doc["legs"][0U].set(legs.first);
    doc["legs"][1U].set(legs.second);
#ifdef PIN_OE
    doc["oe"].set(enable);
#endif // PIN_OE
    doc["offset"].set(legs.first - legs.second);
    const float presetHigh{preset.getHigh()};
    if (presetHigh >= ReferenceHeight::heightLow && presetHigh <= ReferenceHeight::heightHigh)
    {
        doc["preset"]["high"].set(presetHigh);
    }
    const float presetLow{preset.getLow()};
    if (presetLow >= ReferenceHeight::heightLow && presetLow <= ReferenceHeight::heightHigh)
    {
        doc["preset"]["low"].set(presetLow);
    }
    doc["reset"].set(reset);
    doc["rssi"].set(WiFi.RSSI());
    if (lengthRx != 0U)
    {
        doc["rx"].set(toHex(std::span<const uint8_t>(payloadRx).first(lengthRx)));
    }
#ifdef PIN_TPDN
    doc["simulate"]["down"].set(button.getDownSimulation());
#endif // PIN_TPDN
#ifdef PIN_TPUP
    doc["simulate"]["up"].set(button.getUpSimulation());
#endif // PIN_TPUP
    const std::pair<uint8_t, uint8_t> states{leg.getStates()};
    doc["states"][0U].set(states.first);
    doc["states"][1U].set(states.second);
    doc["temperature"].set(temperatureRead());
    doc["tone"]["duration"].set(tone.getDuration());
    doc["tone"]["frequency"].set(tone.getFrequency());
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
