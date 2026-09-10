#pragma once

#ifdef ARDUINO_ARCH_ESP32

#include <ArduinoJson.h> // NOLINT(misc-include-cleaner)
#include <HardwareSerial.h>
#include <span>
#include <string>

class ConsoleHandler
{
public:
    enum class Command : uint8_t
    {
        CALIBRATE = 1U,
        POSITION,
        PRESET_HIGH,
        PRESET_LOW,
        TONE,
    };

    enum class State : uint8_t
    {
        BUTTON_DOWN = 1U,
        BUTTON_UP,
        CONSOLE,
        INITIALIZATION,
        LIN,
        NODE8,
        NODE9,
        POSITION,
        PRESET_HIGH,
        PRESET_LOW,
    };

    /**
     * Initializes console handling.
     */
    void begin();

    void getErrors(JsonArray &errors);

    /**
     * Processes available console input.
     */
    void handle();

    /**
     * Forwards buffered console data.
     */
    void forward();

    void reset();

    void send(Command command);

    void send(Command command, uint16_t value);

private:
    uint8_t errorLin{0U};
    uint8_t errorTx{0U};

    size_t bytesRx{0U};
    size_t bytesTx{0U};
    size_t lengthRx{0U};
    size_t lengthTx{0U};

    std::array<uint8_t, 0b1U << 4U> bufferRx{0U};
    std::array<uint8_t, 0b1U << 4U> bufferTx{0U};

    /**
     * Command to transmit.
     */
    Command commandTx{};

    /**
     * State received from the console.
     */
    State stateRx{};

    static inline hardwareSerial_error_t errorRx{hardwareSerial_error_t::UART_NO_ERROR};

    /**
     * Parses a received console payload.
     */
    void parse();

    void setErrorLin(uint8_t flags);

    void setErrorTx(uint8_t flags);

    void write(std::span<const uint8_t> payload);

    /**
     * Records a hardware serial receive error.
     * @param error Hardware serial error to record.
     */
    static void onReceiveError(hardwareSerial_error_t error);
};

#endif // ARDUINO_ARCH_ESP32