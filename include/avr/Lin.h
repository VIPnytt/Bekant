#pragma once

#ifdef ARDUINO_ARCH_AVR

#include <HardwareSerial.h>
#include <stddef.h> // NOLINT(hicpp-deprecated-headers,modernize-deprecated-headers)
#include <stdint.h> // NOLINT(hicpp-deprecated-headers,modernize-deprecated-headers)
#include <wiring.h>

class Lin
{
private:
    static constexpr unsigned long baud{19'200UL};

    uint8_t addressParity(unsigned int identifier);
    uint8_t calcChecksum(const uint8_t *message, uint8_t nBytes, uint16_t sum);

    int readWithTimeout(int16_t &countDown);

    void serialBreak();

public:
    void begin();
    void send(uint8_t identifier);

    template <size_t N> uint8_t request(uint8_t identifier, uint8_t (&data)[N])
    {
        delay(static_cast<unsigned long>(N) + 1U);
        uint8_t bytesReceived{0U};
        int16_t byte{0U};
        const uint8_t idByte{
            static_cast<uint8_t>((identifier & 0x3FU) | addressParity(static_cast<unsigned int>(identifier)))};
        int16_t countdown{static_cast<int16_t>(124'000'000UL / baud)};
        serialBreak();
        Serial.write(0x55U);
        Serial.write(idByte);
        Serial.flush();
        do
        {
            byte = readWithTimeout(countdown);
        } while (byte != 0x55U && byte != -1);
        do
        {
            byte = readWithTimeout(countdown);
        } while (byte != idByte && byte != -1);
        bytesReceived = 0U;
        for (uint8_t idx{0U}; idx < N; idx++)
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
        if (calcChecksum(data, static_cast<uint8_t>(N), identifier == 0x3DU ? 0U : idByte) != byte)
        {
            bytesReceived = 0xFFU;
        }
        Serial.flush();
        return bytesReceived;
    }

    template <size_t N> void send(uint8_t identifier, const uint8_t (&data)[N])
    {
        const uint8_t addressByte{
            static_cast<uint8_t>((identifier & 0x3FU) | addressParity(static_cast<unsigned int>(identifier)))};
        serialBreak();
        Serial.write(0x55U);
        Serial.write(addressByte);
        Serial.write(static_cast<const uint8_t *>(data), static_cast<uint8_t>(N));
        Serial.write(calcChecksum(
            static_cast<const uint8_t *>(data), static_cast<uint8_t>(N), identifier == 0x3CU ? 0U : addressByte));
        Serial.flush();
        delay(static_cast<unsigned long>(N) + 3U);
    }
};

#endif // ARDUINO_ARCH_AVR
