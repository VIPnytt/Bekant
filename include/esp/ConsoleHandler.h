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

    void parse(std::string message);

    static void onReceiveError(hardwareSerial_error_t error);

public:
    void begin();
    void handle();
    void forward();
    void send(std::string payload);
};

#endif // ARDUINO_ARCH_ESP32