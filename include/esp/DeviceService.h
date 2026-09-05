#pragma once

#ifdef ARDUINO_ARCH_ESP32

#include "esp/ConsoleHandler.h"
#include "esp/IspHandler.h"
#include "esp/MqttHandler.h"
#include "esp/OtaHandler.h"
#include "esp/StatusHandler.h"
#include "esp/WifiHandler.h"

#include <ArduinoJson.h> // NOLINT(misc-include-cleaner)
#include <variant>

class DeviceService
{
private:
    bool buttonDown{false};
    bool buttonUp{false};
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

    std::string payloadRx{};
    std::string payloadTx{};
    std::string versionAvr{};
    std::string versionLatest{};

    std::pair<bool, bool> driveDown{false, false};
    std::pair<bool, bool> driveUp{false, false};

    ConsoleHandler console{};

    IspHandler isp{};

    MqttHandler mqtt{};

    OtaHandler ota{};

    StatusHandler status{};

    WifiHandler wifi{};

    void save();
    void setDriveDown(bool state);
    void setDriveUp(bool state);
    void setOutputEnable(bool state);
    void setReset(bool state);

    [[nodiscard]] float decode(float encoder);

    [[nodiscard]] uint16_t encode(float height);

    [[nodiscard]] std::variant<std::string, std::string_view> printable(std::string_view bytes);

    static void onInterruptDown();
    static void onInterruptReset();
    static void onInterruptUp();

public:
    static constexpr std::string_view version{"1.0.0"};

    void begin();
    void handle();

    void fetchRelease();
    void request(JsonObjectConst doc);
    void safeMode();
    void setButtonDown(bool state);
    void setButtonUp(bool state);
    void setEncoder8(uint16_t position);
    void setEncoder9(uint16_t position);
    void setPresetHigh(uint16_t encoder);
    void setPresetLow(uint16_t encoder);
    void setRx(std::string_view payload);
    void setState8(uint8_t state);
    void setState9(uint8_t state);
    void setTx(std::string_view payload);
    void setVersion(std::string_view version);
    void statusRed();
    void transmit(JsonDocument &doc);

    static DeviceService &getInstance();
};

extern DeviceService &device; // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

#endif // ARDUINO_ARCH_ESP32