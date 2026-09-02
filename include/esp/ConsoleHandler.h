#pragma once

#ifdef ARDUINO_ARCH_ESP32

#include <HardwareSerial.h>
#include <string>

class ConsoleHandler
{
private:
    std::string rxBuffer{};
    std::string txBuffer{};

    /**
 * Parses a received console payload.
 * @param payload Payload received from the console.
 */

/**
 * Records a hardware serial receive error.
 * @param error Hardware serial error to record.
 */

/**
 * Initializes console handling.
 */

/**
 * Processes available console input.
 */

/**
 * Forwards buffered console data.
 */

/**
 * Transmits a console payload.
 * @param payload Payload to transmit.
 */
static inline hardwareSerial_error_t lastError{hardwareSerial_error_t::UART_NO_ERROR};

    void parse(std::string payload);

    static void onReceiveError(hardwareSerial_error_t error);

public:
    void begin();
    void handle();
    void forward();
    void send(std::string payload);
};

#endif // ARDUINO_ARCH_ESP32