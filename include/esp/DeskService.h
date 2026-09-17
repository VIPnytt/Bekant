#pragma once

#ifdef ARDUINO_ARCH_ESP32

#include "esp/ButtonHandler.h"
#include "esp/ConsoleHandler.h"
#include "esp/IspHandler.h"
#include "esp/IssueHandler.h"
#include "esp/MqttHandler.h"
#include "esp/OtaHandler.h"
#include "esp/StatusHandler.h"
#include "esp/ToneHandler.h"
#include "esp/WifiHandler.h"

#include <ArduinoJson.h> // NOLINT(misc-include-cleaner)
#include <span>

class DeskService
{
private:
    bool enable{true};
    bool pending{true};
    bool process{true};
    bool reset{false};
    bool saved{true};

    uint8_t state8{0U};
    uint8_t state9{0U};

    uint16_t encoder8{0U};
    uint16_t encoder9{0U};
    uint16_t presetLow{0U};
    uint16_t presetHigh{0U};

    unsigned long lastMillis{0U};

    size_t lengthRx{0U};
    size_t lengthTx{0U};

    std::string versionLatest{};

    std::array<uint8_t, 0b1U << 4U> payloadRx{};
    std::array<uint8_t, 0b1U << 4U> payloadTx{};

    ButtonHandler button{};

    ConsoleHandler console{};

    IspHandler isp{};

    IssueHandler issue{};

    MqttHandler mqtt{};

    OtaHandler ota{};

    StatusHandler status{};

    ToneHandler tone{};

    WifiHandler wifi{};

    void parseAction(std::string_view action);

    void save();

    void setOutputEnable(bool state);

    void setReset(bool state);

    void statusNode();

    [[nodiscard]] float decode(float encoder);

    [[nodiscard]] uint16_t encode(float height);

    [[nodiscard]] std::string toHex(std::span<const uint8_t> payload);

    static void onReset();

public:
    static constexpr std::string_view version{"1.0.0"};

    void begin();

    void handle();

    void fetchRelease();

    void request(JsonObjectConst doc);

    void safeMode();

    void setNode8(uint16_t position, uint8_t state);

    void setNode9(uint16_t position, uint8_t state);

    void setPending();

    void setPresetHigh(uint16_t encoder);

    void setPresetLow(uint16_t encoder);

    void setRx(std::span<const uint8_t> payload);

    void setTx(std::span<const uint8_t> payload);

    void transmit(JsonDocument &doc);

    static DeskService &getInstance();
};

extern DeskService &desk; // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

#endif // ARDUINO_ARCH_ESP32