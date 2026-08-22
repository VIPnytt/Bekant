#ifdef ARDUINO_ARCH_AVR

#include "avr/Lin.h"

#include "avr/constants.h"

#include <HardwareSerial.h>
#include <wiring.h>

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
    while (sum >> 8U)
    {
        sum = (sum & 0xFFU) + (sum >> 8U);
    }
    return ~sum;
}

uint8_t Lin::addressParity(uint8_t address)
{
    const uint8_t p0 =
        ((address >> 0U) & 1U) ^ ((address >> 1U) & 1U) ^ ((address >> 2U) & 1U) ^ ((address >> 4U) & 1U);
    const uint8_t p1 =
        ~(((address >> 1U) & 1U) ^ ((address >> 3U) & 1U) ^ ((address >> 4U) & 1U) ^ ((address >> 5U) & 1U));
    return (p0 | (p1 << 1U)) << 6U;
}

int Lin::readWithTimeout(int16_t &countdown)
{
    while (!Serial.available())
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

uint8_t Lin::request(uint8_t address, uint8_t *data, uint8_t length)
{
    delay(length + 1U);
    uint8_t bytesReceived{0U};
    int16_t byte{0U};
    const uint8_t idByte{static_cast<uint8_t>((address & 0x3FU) | addressParity(address))};
    int16_t countdown{static_cast<int16_t>(124'000'000UL / baud)};
    serialBreak();
    Serial.write(0x55U);
    Serial.write(idByte);
    Serial.flush();
    bytesReceived = 0xFDU;
    do
    {
        byte = readWithTimeout(countdown);
    } while (byte != 0x55U && byte != -1);
    bytesReceived = 0xFEU;
    do
    {
        byte = readWithTimeout(countdown);
    } while (byte != idByte && byte != -1);
    bytesReceived = 0U;
    for (uint8_t idx{0U}; idx < length; idx++)
    {
        byte = readWithTimeout(countdown);
        data[idx] = byte;
        if (byte == -1)
        {
            Serial.flush();
            return bytesReceived;
        }
        ++bytesReceived;
    }
    byte = readWithTimeout(countdown);
    ++bytesReceived;
    if (calcChecksum(data, length, address == 0x3DU ? 0U : idByte) != byte)
    {
        bytesReceived = 0xFFU;
    }
    Serial.flush();
    return bytesReceived;
}

void Lin::send(uint8_t address, const uint8_t *data, uint8_t length)
{
    const uint8_t addressByte = (address & 0x3FU) | addressParity(address);
    serialBreak();
    Serial.write(0x55U);
    Serial.write(addressByte);
    Serial.write(data, length);
    Serial.write(calcChecksum(data, length, address == 0x3CU ? 0U : addressByte));
    Serial.flush();
    delay(length + 3U);
}

#endif // ARDUINO_ARCH_AVR
