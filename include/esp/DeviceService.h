#pragma once

#ifdef ARDUINO_ARCH_ESP32

#include "esp/ConsoleHandler.h"
#include "esp/IspHandler.h"
#include "esp/MqttHandler.h"
#include "esp/OtaHandler.h"
#include "esp/StatusHandler.h"
#include "esp/WifiHandler.h"

#include <ArduinoJson.h> // NOLINT(misc-include-cleaner)

class DeviceService
{
private:
    bool buttonDown{false};
    bool buttonUp{false};
    bool enable{true};
    bool reset{false};
    bool pending{true};
    bool process{true};
    bool saved{true};

    unsigned long lastMillis{0U};

    uint16_t encoderA{0U};
    uint16_t encoderB{0U};
    uint16_t presetLow{0U};
    uint16_t presetHigh{0U};

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

    static void onInterruptDown();
    static void onInterruptReset();
    static void onInterruptUp();

public:
    static constexpr std::string_view version{"1.0.0"};

    void begin();
    void handle();

    void fetchRelease();
    void safeMode();
    void request(JsonObjectConst doc);
    void setButtonDown(bool state);
    void setButtonUp(bool state);
    void setEncoderA(uint16_t encoder);
    void setEncoderB(uint16_t encoder);
    void setPresetHigh(uint16_t encoder);
    void setPresetLow(uint16_t encoder);
    void setRx(std::string payload);
    void setTx(std::string payload);
    void setVersion(std::string_view version);
    void statusRed();
    void transmit(JsonDocument &doc);

    static DeviceService &getInstance();
};

extern DeviceService &device; // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

#endif // ARDUINO_ARCH_ESP32