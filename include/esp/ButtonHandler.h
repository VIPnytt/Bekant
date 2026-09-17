#pragma once

#ifdef ARDUINO_ARCH_ESP32

#include <ArduinoJson.h> // NOLINT(misc-include-cleaner)

class ButtonHandler
{
private:
    static inline uint8_t states{0U};

    static inline std::pair<bool, bool> simulateDown{false, false};
    static inline std::pair<bool, bool> simulateUp{false, false};

    static void onDown();

    static void onUp();

public:
    void begin();

    bool getDown() const;

    bool getDownSimulation() const;

    bool getState3() const;

    bool getState4() const;

    bool getUp() const;

    bool getUpSimulation() const;

    void resetSimulation();

    void setSimulateDown(bool state);

    void setSimulateUp(bool state);

    void setStatus();

    static void setStates(uint8_t flags);
};

#endif // ARDUINO_ARCH_ESP32
