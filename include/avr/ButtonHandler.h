#pragma once

#ifdef ARDUINO_ARCH_AVR

class ButtonHandler
{
private:
    bool stateDown{false};
    bool stateUp{false};

    /**
 * Tracks the accumulated button event count.
 */
signed char count{0};

    /**
 * Handles the current button input state.
 */
unsigned long lastMillis{0U};

    void cancel();

    void incrementDown();

    void incrementUp();

    void process();

public:
    void handle();
};

#endif // ARDUINO_ARCH_AVR
