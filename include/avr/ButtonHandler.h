#pragma once

#ifdef ARDUINO_ARCH_AVR

class ButtonHandler
{
private:
    /**
     * Handles the current button-down input state.
     */

    bool stateDown{false};
    /**
     * Handles the current button-up input state.
     */
    bool stateUp{false};

    /**
     * Tracks the accumulated button event count.
     */
    signed char count{0};

    unsigned long lastMillis{0U};

    void cancel();

    void incrementDown();

    void incrementUp();

    void process();

public:
    void handle();
};

#endif // ARDUINO_ARCH_AVR
