#pragma once

#ifdef ARDUINO_ARCH_AVR

#include <inttypes.h>

class Lin
{
private:
    static constexpr unsigned long baud{19'200UL};

    uint8_t addressParity(uint8_t addr);
    uint8_t calcChecksum(const uint8_t *message, uint8_t nBytes, uint16_t start = 0);

    int readWithTimeout(int16_t &countDown);

    void serialBreak();

public:
    void begin();
    void send(uint8_t addr, const uint8_t *message, uint8_t nBytes);

    uint8_t request(uint8_t addr, uint8_t *message, uint8_t nBytes);
};

#endif // ARDUINO_ARCH_AVR
