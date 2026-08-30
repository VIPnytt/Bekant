#ifdef ARDUINO_ARCH_ESP32

#include "esp/DeviceService.h"

#include "esp/constants.h" // NOLINT(misc-include-cleaner)

#include <WiFi.h> // NOLINT(misc-include-cleaner)
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
 * @brief Decodes an encoder value into a physical desk height.
 *
 * @param encoder Encoded 16-bit encoder value.
 */
float DeviceService::decode(float encoder)
{
    return ((encoder - static_cast<float>(ReferenceHeight::encoderLow)) *
            (ReferenceHeight::heightHigh - ReferenceHeight::heightLow) /
            static_cast<float>(ReferenceHeight::encoderHigh - ReferenceHeight::encoderLow)) +
           ReferenceHeight::heightLow;
}

uint16_t DeviceService::encode(float height)
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
 * Handles desk actions, button controls, height and preset commands, output-enable and reset state changes, and raw
 * transmissions.
 *
 * @param doc JSON object containing the requested commands.
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
    if (doc["desk"].is<float>())
    {
        status.setWhite();
        console.send("e" + std::to_string(static_cast<int>(encode(doc["desk"].as<float>()))));
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
    if (doc["preset"]["high"].is<float>())
    {
        status.setWhite();
        console.send("h" + std::to_string(static_cast<int>(encode(doc["preset"]["high"].as<float>()))));
    }
    if (doc["preset"]["low"].is<bool>() && doc["preset"]["low"].as<bool>())
    {
        status.setWhite();
        console.send("l");
    }
    if (doc["preset"]["low"].is<float>())
    {
        status.setWhite();
        console.send("l" + std::to_string(static_cast<int>(encode(doc["preset"]["low"].as<float>()))));
    }
    if (doc["reset"].is<bool>())
    {
        device.setReset(doc["reset"].as<bool>());
    }
    if (doc["tx"].is<std::string>())
    {
        status.setWhite();
        console.send(doc["tx"].as<std::string>());
    }
}

void DeviceService::safeMode()
{
    process = false;
    Serial1.end();
    mqtt.disconnect();
}

/**
 * @brief Persists unsaved encoder and preset values to non-volatile storage.
 */
void DeviceService::save()
{
    nvs_handle_t handle{};
    if (nvs_open("bekant", nvs_open_mode_t::NVS_READWRITE, &handle) == ESP_OK)
    {
        nvs_set_u16(handle, "a", encoderA);
        nvs_set_u16(handle, "b", encoderB);
        nvs_set_u16(handle, "h", presetHigh);
        nvs_set_u16(handle, "l", presetLow);
        nvs_set_u8(handle, "oe", static_cast<uint8_t>(enable));
        saved = nvs_commit(handle) == ESP_OK;
        nvs_close(handle);
    }
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
#ifdef PIN_TPDN
    doc["button"]["down"].set(buttonDown || driveDown.first);
#else
    doc["button"]["down"].set(buttonDown);
#endif // PIN_TPDN
#ifdef PIN_TPUP
    doc["button"]["up"].set(buttonUp || driveUp.first);
#else
    doc["button"]["up"].set(buttonUp);
#endif // PIN_TPUP
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
#ifdef PIN_OE
    doc["oe"].set(enable);
#endif // PIN_OE
    if (presetHigh != 0U)
    {
        doc["preset"]["high"].set(decode(static_cast<float>(presetHigh)));
    }
    if (presetLow != 0U)
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

void DeviceService::setButtonDown(bool state)
{
    buttonDown = state;
    status.setWhite();
    pending = true;
}

void DeviceService::setButtonUp(bool state)
{
    buttonUp = state;
    status.setWhite();
    pending = true;
}

/**
 * @brief Sets the optional desk down-button control state.
 *
 * Pressing the button sets the status indicator to red.
 *
 * @param state Whether the down button should be active.
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
 * @brief Sets the optional up button control state.
 *
 * @param state `true` to activate the up button and `false` to release it.
 */
void DeviceService::setDriveUp(bool state)
{
#ifdef PIN_TPUP
    driveUp.first = state;
    status.setRed();
    digitalWrite(PIN_TPUP, driveUp.first ? LOW : HIGH);
#endif // PIN_TPUP
}

void DeviceService::setEncoderA(uint16_t state)
{
    ((buttonDown && !buttonUp && !driveDown.first && !driveUp.first) ||
     (buttonUp && !buttonDown && !driveDown.first && !driveUp.first))
        ? status.setGreen()
        : status.setBlue();
    encoderA = state;
    pending = true;
}

void DeviceService::setEncoderB(uint16_t state)
{
    ((buttonDown && !buttonUp && !driveDown.first && !driveUp.first) ||
     (buttonUp && !buttonDown && !driveDown.first && !driveUp.first))
        ? status.setGreen()
        : status.setBlue();
    encoderB = state;
    pending = true;
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
    if (state != enable)
    {
        enable = state;
        status.setNone();
        digitalWrite(PIN_OE, enable ? HIGH : LOW);
        pending = true;
    }
#endif // PIN_OE
}

void DeviceService::setPresetHigh(uint16_t preset)
{
    presetHigh = preset;
    pending = true;
}

void DeviceService::setPresetLow(uint16_t preset)
{
    presetLow = preset;
    pending = true;
}

/**
 * @brief Sets the desk reset state and publishes updated metadata when it changes.
 *
 * @param state Whether reset should be asserted.
 */
void DeviceService::setReset(bool state) { digitalWrite(PIN_RST, state ? LOW : HIGH); }

void DeviceService::setRx(std::string payload)
{
    payloadRx = payload;
    pending = true;
}

void DeviceService::setTx(std::string payload)
{
    payloadTx = payload;
    pending = true;
}

void DeviceService::statusRed() { status.setRed(); }

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

void DeviceService::onInterruptReset()
{
    device.reset = digitalRead(PIN_RST) == LOW;
    device.reset ? device.status.setNone(true) : device.status.setWhite();
    device.pending = true;
}

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
