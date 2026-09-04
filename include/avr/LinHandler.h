#pragma once

#ifdef ARDUINO_ARCH_AVR

#include <HardwareSerial.h>
#include <wiring.h>

namespace LinFrame
{
static constexpr unsigned char breakBits{13U};
static constexpr unsigned char delimiterBits{1U};
static constexpr unsigned char syncBits{10U};
static constexpr unsigned char identifierBits{10U};
static constexpr unsigned char headerBits{breakBits + delimiterBits + syncBits + identifierBits};
static constexpr unsigned char dataBits{80U};
static constexpr unsigned char checksumBits{10U};
static constexpr unsigned char frameBits{headerBits + dataBits + checksumBits};
} // namespace LinFrame

class LinHandler
{
private:
    static constexpr unsigned long baud{19'200UL};

    void serialBreak();

    unsigned char addressParity(unsigned int identifier);

    int readWithTimeout(unsigned int &remainingTime);

    /**
     * Calculates the LIN checksum for a data array.
     * @param data Bytes included in the checksum.
     * @param sum Initial checksum sum.
     * @returns The bitwise-complemented LIN checksum.
     */
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
    }

    template <unsigned int N> unsigned char request(unsigned char identifier, unsigned char (&data)[N])
    {
        const unsigned char idByte{
            static_cast<unsigned char>((identifier & 0x3FU) | addressParity(static_cast<unsigned int>(identifier)))};
        serialBreak();
        Serial.write(0x55U);
        Serial.write(idByte);
        Serial.flush();
        int byte{0U};
        unsigned int remainingTime{static_cast<unsigned int>(LinFrame::frameBits * 1'000'000UL / baud)};
        do
        {
            byte = readWithTimeout(remainingTime);
        } while (byte != -1 && byte != 0x55);
        do
        {
            byte = readWithTimeout(remainingTime);
        } while (byte != -1 && byte != idByte);
        unsigned char count{0U};
        for (unsigned char &byte : data)
        {
            byte = readWithTimeout(remainingTime);
            if (byte == -1)
            {
                Serial.flush();
                return count;
            }
            byte = static_cast<unsigned char>(byte);
            ++count;
        }
        byte = readWithTimeout(remainingTime);
        ++count;
        if (calcChecksum(data, identifier == 0x3DU ? 0U : idByte) != byte)
        {
            count = 0xFFU;
        }
        Serial.flush();
        return count;
    }
};

#endif // ARDUINO_ARCH_AVR
