#pragma once

#ifdef ARDUINO_ARCH_ESP32

#include <HardwareSerial.h>
#include <string>

class ConsoleHandler
{
public:
    enum class Command : uint8_t
    {
        BUTTON_DOWN,
        BUTTON_UP,
        ENCODER8,
        ENCODER9,
        INITIALIZE,
        NODE8,
        NODE9,
        POSITION,
        PRESET_HIGH,
        PRESET_LOW,
        STATE8,
        STATE9,
        TONE,
    };

    /**
     * Initializes console handling.
     */
    void begin();

    /**
     * Processes available console input.
     */
    void handle();

    /**
     * Forwards buffered console data.
     */
    void forward();

    /**
     * Transmits a console payload.
     * @param payload Payload to transmit.
     */
    void send(std::string_view payload);

private:
    uint8_t rxBytes{0U};
    uint8_t rxCommand{0U};
    uint8_t rxLength{0U};

    std::array<uint8_t, 0b1U << 4U> rxBuffer{};

    std::string txBuffer{};

    static inline hardwareSerial_error_t lastError{hardwareSerial_error_t::UART_NO_ERROR};

    /**
     * Parses a received console payload.
     */
    void parse() const;

    /**
     * Records a hardware serial receive error.
     * @param error Hardware serial error to record.
     */
    static void onReceiveError(hardwareSerial_error_t error);
};

#endif // ARDUINO_ARCH_ESP32