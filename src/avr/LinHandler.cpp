#ifdef ARDUINO_ARCH_AVR

#include "avr/LinHandler.h"

#include "avr/constants.h"

void LinHandler::begin()
{
    pinMode(Pin::lin, OUTPUT);
    Serial.begin(baud);
}

void LinHandler::serialBreak()
{
    Serial.end();
    pinMode(Pin::lin, OUTPUT);
    digitalWrite(Pin::lin, LOW);
    delayMicroseconds(static_cast<unsigned int>(15'000'000UL / baud));
    digitalWrite(Pin::lin, HIGH);
    delayMicroseconds(static_cast<unsigned int>(1'000'000UL / baud));
    Serial.begin(baud);
}

uint8_t LinHandler::addressParity(unsigned int identifier)
{
    const unsigned int parity0{((identifier >> 0U) & 1U) ^ ((identifier >> 1U) & 1U) ^ ((identifier >> 2U) & 1U) ^
                               ((identifier >> 4U) & 1U)};
    const unsigned int parity1{~(((identifier >> 1U) & 1U) ^ ((identifier >> 3U) & 1U) ^ ((identifier >> 4U) & 1U) ^
                                 ((identifier >> 5U) & 1U)) &
                               1U};
    return static_cast<uint8_t>((parity0 | (parity1 << 1U)) << 6U);
}

int LinHandler::readWithTimeout(int16_t &countdown)
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

void LinHandler::send(uint8_t identifier)
{
    const uint8_t addressByte{
        static_cast<uint8_t>((identifier & 0x3FU) | addressParity(static_cast<unsigned int>(identifier)))};
    serialBreak();
    Serial.write(0x55U);
    Serial.write(addressByte);
    Serial.write(identifier == 0x3CU ? 0xFFU : static_cast<uint8_t>(~addressByte));
    Serial.flush();
    delay(3U);
}

#endif // ARDUINO_ARCH_AVR
