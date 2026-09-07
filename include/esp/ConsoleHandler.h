#pragma once

#ifdef ARDUINO_ARCH_ESP32

#include <HardwareSerial.h>
#include <span>
#include <string>

class ConsoleHandler
{
public:
    enum class Command : uint8_t
    {
        BUTTON_DOWN,
        BUTTON_UP,
        CALIBRATE,
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

    void send(Command command);
    void send(Command command, uint16_t value);

private:
    uint8_t commandRx{0U};
    uint8_t commandTx{0U};

    size_t bytesRx{0U};
    size_t bytesTx{0U};
    size_t lengthRx{0U};
    size_t lengthTx{0U};

    std::array<uint8_t, 0b1U << 4U> bufferRx{0U};
    std::array<uint8_t, 0b1U << 4U> bufferTx{0U};

    static inline hardwareSerial_error_t lastError{hardwareSerial_error_t::UART_NO_ERROR};

    /**
     * Parses a received console payload.
     */
    void parse() const;
    void write(std::span<const uint8_t> payload);

    /**
     * Records a hardware serial receive error.
     * @param error Hardware serial error to record.
     */
    static void onReceiveError(hardwareSerial_error_t error);
};

#endif // ARDUINO_ARCH_ESP32