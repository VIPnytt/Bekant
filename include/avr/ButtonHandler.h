#pragma once

#ifdef __AVR__

#ifdef __AVR_ATtiny841__
#include <core_pins.h>
#elif defined(__AVR_ATtiny1624__)
#include <pins_arduino.h>
#endif // __AVR_ATtiny841__

class ButtonHandler
{
private:
#ifdef __AVR_ATtiny841__
    static constexpr unsigned char pinDown{PIN_PB1};
    static constexpr unsigned char pinUp{PIN_PB0};
#elif defined(__AVR_ATtiny1624__)
    static constexpr unsigned char pinDown{PIN_PA6};
    static constexpr unsigned char pinUp{PIN_PA5};
#endif // __AVR_ATtiny841__

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

    void incrementDown();

    void incrementUp();

    void process();

    /**
     * Finalizes manual movement when a button is released.
     */
    void stop();

public:
    void begin();

    void handle();
};

#endif // __AVR__
