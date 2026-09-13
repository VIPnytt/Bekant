#pragma once

#ifdef ARDUINO_ARCH_ESP32

#include "esp/ConsoleHandler.h"
#include "esp/IspHandler.h"
#include "esp/MqttHandler.h"
#include "esp/OtaHandler.h"
#include "esp/StatusHandler.h"
#include "esp/WifiHandler.h"

#include <ArduinoJson.h> // NOLINT(misc-include-cleaner)
#include <span>

class DeskService
{
private:
    /**
     * Computes the fingerprint used to compare firmware versions.
     *
     * @param characters Version characters to fingerprint.
     * @return The 8-bit firmware fingerprint.
     */
    [[nodiscard]] constexpr uint8_t fingerprint(std::string_view characters)
    {
        uint8_t hash{0U}; // NOLINT(misc-const-correctness)
        for (const char character : characters)
        {
            hash ^= static_cast<uint8_t>(character);
            hash = static_cast<uint8_t>((hash << 3U) | (hash >> 5U));
        }
        return hash;
    }

    bool buttonDown{false};
    bool buttonUp{false};
    bool enable{true};
    bool pending{true};
    bool process{true};
    bool reset{false};
    bool saved{true};

    uint8_t error8{0U};
    uint8_t error9{0U};
    uint8_t errorInit{0U};
    uint8_t state8{0U};
    uint8_t state9{0U};
    uint8_t versionAvr{0U};

    uint16_t encoder8{0U};
    uint16_t encoder9{0U};
    uint16_t presetLow{0U};
    uint16_t presetHigh{0U};

    unsigned long lastMillis{0U};

    size_t lengthRx{0U};
    size_t lengthTx{0U};

    hardwareSerial_error_t errorRx{hardwareSerial_error_t::UART_NO_ERROR};

    std::string versionLatest{};

    std::array<uint8_t, 0b1U << 4U> payloadRx{};
    std::array<uint8_t, 0b1U << 4U> payloadTx{};

    std::pair<bool, bool> driveDown{false, false};
    std::pair<bool, bool> driveUp{false, false};

    ConsoleHandler console{};

    IspHandler isp{};

    MqttHandler mqtt{};

    OtaHandler ota{};

    StatusHandler status{};

    WifiHandler wifi{};

    void getErrors(JsonArray &list);

    void save();

    void setDriveDown(bool state);

    void setDriveUp(bool state);

    void setOutputEnable(bool state);

    void setReset(bool state);

    void statusNode();

    [[nodiscard]] float decode(float encoder);

    [[nodiscard]] uint16_t encode(float height);

    [[nodiscard]] std::string toHex(std::span<const uint8_t> payload);

    static void onInterruptDown();

    static void onInterruptReset();

    static void onInterruptUp();

public:
    static constexpr std::string_view version{"1.0.0"};

    void begin();

    void fetchRelease();

    void handle();

    void request(JsonObjectConst doc);

    void safeMode();

    void setButtonDown(bool state);

    void setButtonUp(bool state);

    void setError8(uint8_t flags);

    void setError9(uint8_t flags);

    void setErrorInit(uint8_t flags);

    void setNode8(uint16_t position, uint8_t state);

    void setNode9(uint16_t position, uint8_t state);

    void setPending();

    void setPresetHigh(uint16_t encoder);

    void setPresetLow(uint16_t encoder);

    void setRx(std::span<const uint8_t> payload);

    void setTx(std::span<const uint8_t> payload);

    void setVersion(uint8_t hash);

    void transmit(JsonDocument &doc);

    static DeskService &getInstance();
};

extern DeskService &desk; // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

#endif // ARDUINO_ARCH_ESP32