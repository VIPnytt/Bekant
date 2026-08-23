#ifdef ARDUINO_ARCH_AVR

#include "avr/Lin.h"

#include "avr/constants.h"

void Lin::begin()
{
    pinMode(Pin::lin, OUTPUT);
    Serial.begin(baud);
}

void Lin::serialBreak()
{
    Serial.end();
    pinMode(Pin::lin, OUTPUT);
    digitalWrite(Pin::lin, LOW);
    delayMicroseconds(static_cast<unsigned int>(15'000'000UL / baud));
    digitalWrite(Pin::lin, HIGH);
    delayMicroseconds(static_cast<unsigned int>(1'000'000UL / baud));
    Serial.begin(baud);
}

uint8_t Lin::calcChecksum(const uint8_t *data, uint8_t length, uint16_t sum)
{
    while (length-- > 0)
    {
        sum += *(data++);
    }
    while ((sum >> 8U) != 0U)
    {
        sum = (sum & 0xFFU) + (sum >> 8U);
    }
    return ~sum;
}

uint8_t Lin::addressParity(uint8_t identifier)
{
    const auto parity0{((identifier >> 0U) & 1U) ^ ((identifier >> 1U) & 1U) ^ ((identifier >> 2U) & 1U) ^
                       ((identifier >> 4U) & 1U)};
    const auto parity1{~(((identifier >> 1U) & 1U) ^ ((identifier >> 3U) & 1U) ^ ((identifier >> 4U) & 1U) ^
                         ((identifier >> 5U) & 1U)) &
                       1U};
    return static_cast<uint8_t>((parity0 | (parity1 << 1U)) << 6U);
}

int Lin::readWithTimeout(int16_t &countdown)
{
    while (Serial.available() == 0)
    {
        delayMicroseconds(100U);
        countdown -= 100;
        if (countdown <= 0)
        {
            return -1;
        }
    }
    return Serial.read();
}

void Lin::send(uint8_t identifier)
{
    const uint8_t addressByte{static_cast<uint8_t>((identifier & 0x3FU) | addressParity(identifier))};
    serialBreak();
    Serial.write(0x55U);
    Serial.write(addressByte);
    Serial.write(calcChecksum(nullptr, 0U, identifier == 0x3CU ? 0U : addressByte));
    Serial.flush();
    delay(3U);
}

#endif // ARDUINO_ARCH_AVR
