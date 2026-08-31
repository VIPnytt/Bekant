#pragma once

#ifdef ARDUINO_ARCH_ESP32

#include <HardwareSerial.h>
#include <string>

class ConsoleHandler
{
private:
    std::string buffer{};

    /**
 * Processes a received console message.
 * @param message Message received from the console.
 */

/**
 * Handles a hardware serial receive error.
 * @param error Hardware serial error to record.
 */

/**
 * Initializes the console handler.
 */

/**
 * Processes available console input.
 */

/**
 * Transmits a payload through the console.
 * @param payload Data to transmit.
 */
static inline hardwareSerial_error_t lastError{hardwareSerial_error_t::UART_NO_ERROR};

    void parse(std::string message);

    static void onReceiveError(hardwareSerial_error_t error);

public:
    void begin();
    void handle();
    void send(std::string payload);
};

#endif // ARDUINO_ARCH_ESP32