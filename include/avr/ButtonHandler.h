#pragma once

#ifdef ARDUINO_ARCH_AVR

class ButtonHandler
{
private:
    bool stateDown{false};
    bool stateUp{false};

    char count{0};

    /**
 * Handles button input and updates the button state.
 */
unsigned long lastMillis{0U};

    void process();
    void incrementDown();
    void incrementUp();
    void cancel();

public:
    void handle();
};

#endif // ARDUINO_ARCH_AVR
