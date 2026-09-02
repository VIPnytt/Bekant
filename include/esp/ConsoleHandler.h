#pragma once

#ifdef ARDUINO_ARCH_ESP32

#include <HardwareSerial.h>
#include <string>

class ConsoleHandler
{
private:
    std::string rxBuffer{};
    std::string txBuffer{};

    static inline hardwareSerial_error_t lastError{hardwareSerial_error_t::UART_NO_ERROR};

    /**
     * Parses a received console payload.
     * @param payload Payload received from the console.
     */
    void parse(std::string payload);

    /**
     * Records a hardware serial receive error.
     * @param error Hardware serial error to record.
     */
    static void onReceiveError(hardwareSerial_error_t error);

public:
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
    void send(std::string payload);
};

#endif // ARDUINO_ARCH_ESP32