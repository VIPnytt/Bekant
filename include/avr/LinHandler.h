#pragma once

#ifdef ARDUINO_ARCH_AVR

#include <HardwareSerial.h>
#include <wiring.h>

class LinHandler
{
private:
    static constexpr unsigned long baud{19'200UL};

    void serialBreak();

    unsigned char addressParity(unsigned int identifier);

    int readWithTimeout(int16_t &countDown);

    template <unsigned int N> unsigned char calcChecksum(const unsigned char (&data)[N], unsigned int sum)
    {
        for (const unsigned char byte : data)
        {
            sum += byte;
        }
        while ((sum >> 8U) != 0U)
        {
            sum = (sum & 0xFFU) + (sum >> 8U);
        }
        return static_cast<unsigned char>(~sum);
    }

public:
    void begin();
    void send(unsigned char identifier);

    template <unsigned int N> void send(unsigned char identifier, const unsigned char (&data)[N])
    {
        const unsigned char addressByte{
            static_cast<unsigned char>((identifier & 0x3FU) | addressParity(static_cast<unsigned int>(identifier)))};
        serialBreak();
        Serial.write(0x55U);
        Serial.write(addressByte);
        Serial.write(data, N);
        Serial.write(calcChecksum(data, identifier == 0x3CU ? 0U : addressByte));
        Serial.flush();
        delay(static_cast<unsigned long>(N) + 3UL);
    }

    template <unsigned int N> unsigned char request(unsigned char identifier, unsigned char (&data)[N])
    {
        delay(static_cast<unsigned long>(N) + 1U);
        unsigned char bytesReceived{0U};
        int _byte{0U};
        const unsigned char idByte{
            static_cast<unsigned char>((identifier & 0x3FU) | addressParity(static_cast<unsigned int>(identifier)))};
        int16_t countdown{static_cast<int16_t>(124'000'000UL / baud)};
        serialBreak();
        Serial.write(0x55U);
        Serial.write(idByte);
        Serial.flush();
        do
        {
            _byte = readWithTimeout(countdown);
        } while (_byte != -1 && _byte != 0x55);
        do
        {
            _byte = readWithTimeout(countdown);
        } while (_byte != -1 && _byte != idByte);
        bytesReceived = 0U;
        for (unsigned char &byte : data)
        {
            _byte = readWithTimeout(countdown);
            if (_byte == -1)
            {
                Serial.flush();
                return bytesReceived;
            }
            byte = static_cast<unsigned char>(_byte);
            ++bytesReceived;
        }
        _byte = readWithTimeout(countdown);
        ++bytesReceived;
        if (calcChecksum(data, identifier == 0x3DU ? 0U : idByte) != _byte)
        {
            bytesReceived = 0xFFU;
        }
        Serial.flush();
        return bytesReceived;
    }
};

#endif // ARDUINO_ARCH_AVR
