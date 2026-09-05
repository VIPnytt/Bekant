#pragma once

#ifdef ARDUINO_ARCH_AVR

#include <HardwareSerial.h>

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
     * Calculates the complemented checksum for a sequence of bytes.
     * @param data Bytes to include in the checksum.
     * @param sum Initial checksum sum.
     * @return The complemented checksum.
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

    /**
     * Requests a LIN frame and stores its payload in the provided buffer.
     *
     * @param identifier LIN frame identifier to request.
     * @param data Buffer to receive the frame payload.
     * @return `true` if a complete frame with a valid checksum is received, `false` on timeout or checksum failure.
     */
    template <unsigned int N> bool request(unsigned char identifier, unsigned char (&data)[N])
    {
        const unsigned char idByte{
            static_cast<unsigned char>((identifier & 0x3FU) | addressParity(static_cast<unsigned int>(identifier)))};
        serialBreak();
        Serial.write(0x55U);
        Serial.write(idByte);
        Serial.flush();
        int receivedByte{0};
        unsigned int remainingTime{static_cast<unsigned int>(LinFrame::frameBits * 1'000'000UL / baud)};
        do // NOLINT(cppcoreguidelines-avoid-do-while)
        {
            receivedByte = readWithTimeout(remainingTime);
        } while (receivedByte != -1 && receivedByte != 0x55);
        if (receivedByte == -1)
        {
            return false;
        }
        do // NOLINT(cppcoreguidelines-avoid-do-while)
        {
            receivedByte = readWithTimeout(remainingTime);
        } while (receivedByte != -1 && receivedByte != idByte);
        if (receivedByte == -1)
        {
            return false;
        }
        for (unsigned char &dataByte : data)
        {
            receivedByte = readWithTimeout(remainingTime);
            if (receivedByte == -1)
            {
                return false;
            }
            dataByte = static_cast<unsigned char>(receivedByte);
        }
        receivedByte = readWithTimeout(remainingTime);
        return receivedByte != -1 && calcChecksum(data, identifier == 0x3DU ? 0U : idByte) == receivedByte;
    }
};

#endif // ARDUINO_ARCH_AVR
