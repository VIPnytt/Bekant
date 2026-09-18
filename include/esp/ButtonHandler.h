#pragma once

#ifdef ARDUINO_ARCH_ESP32

#include <ArduinoJson.h> // NOLINT(misc-include-cleaner)
#include <utility>

class ButtonHandler
{
private:
    uint8_t states{0U};

    static inline std::pair<bool, bool> simulateDown{false, false};
    static inline std::pair<bool, bool> simulateUp{false, false};

    static void onDown();

    static void onUp();

public:
    void begin();

    void resetSimulation();

    void setSimulateDown(bool state);

    void setSimulateUp(bool state);

    void setStates(uint8_t flags);

    void setStatus();

    [[nodiscard]] bool getDown();

    [[nodiscard]] bool getDownSimulation();

    [[nodiscard]] bool getState3() const;

    [[nodiscard]] bool getState4() const;

    [[nodiscard]] bool getUp();

    [[nodiscard]] bool getUpSimulation();
};

#endif // ARDUINO_ARCH_ESP32
