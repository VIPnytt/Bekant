#pragma once

#ifdef ARDUINO_ARCH_ESP32

#include "esp/ButtonHandler.h"
#include "esp/ConsoleHandler.h"
#include "esp/IspHandler.h"
#include "esp/IssueHandler.h"
#include "esp/LegHandler.h"
#include "esp/MqttHandler.h"
#include "esp/OtaHandler.h"
#include "esp/PresetHandler.h"
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

    unsigned long lastMillis{0UL};

    size_t lengthRx{0U};
    size_t lengthTx{0U};

    std::string versionLatest{};

    std::array<uint8_t, 0b1U << 4U> payloadRx{};
    std::array<uint8_t, 0b1U << 4U> payloadTx{};

    ButtonHandler button{};

    ConsoleHandler console{};

    IspHandler isp{};

    IssueHandler issue{};

    LegHandler leg{};

    MqttHandler mqtt{};

    OtaHandler ota{};

    PresetHandler preset{};

    StatusHandler status{};

    ToneHandler tone{};

    WifiHandler wifi{};

    void parseAction(std::string_view action);

    void save();

    void setOutputEnable(bool state);

    void setReset(bool state);

    void setRx(std::span<const uint8_t> payload);

    [[nodiscard]] std::string toHex(std::span<const uint8_t> payload);

    static void onReset();

public:
    static constexpr std::array<esp_reset_reason_t, 7U> resetAbnormalities{
        esp_reset_reason_t::ESP_RST_BROWNOUT,
        esp_reset_reason_t::ESP_RST_CPU_LOCKUP,
        esp_reset_reason_t::ESP_RST_INT_WDT,
        esp_reset_reason_t::ESP_RST_PANIC,
        esp_reset_reason_t::ESP_RST_PWR_GLITCH,
        esp_reset_reason_t::ESP_RST_TASK_WDT,
        esp_reset_reason_t::ESP_RST_WDT,
    };

    static constexpr std::string_view version{"1.0.0"};

    void begin();

    void handle();

    void getRelease();

    void parse(ConsoleHandler::State stateRx, std::span<const uint8_t> payload);

    void parse(JsonObjectConst doc);

    void safeMode();

    void setPending();

    void setStatus();

    void setTx(std::span<const uint8_t> payload);

    void transmit(JsonDocument &doc);

    [[nodiscard]] static float decode(float encoder);

    [[nodiscard]] static uint16_t encode(float height);

    static DeskService &getInstance();
};

extern DeskService &desk; // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

#endif // ARDUINO_ARCH_ESP32
