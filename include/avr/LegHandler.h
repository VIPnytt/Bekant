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

class LegHandler
{
private:
    static constexpr unsigned long baudRate{19'200UL};
    static constexpr unsigned char linDiagnosticRequestId{0x3CU};
    static constexpr unsigned char linDiagnosticResponseId{0x3DU};
    static constexpr unsigned char linSyncByte{0x55U};

    void serialBreak();

    [[nodiscard]] unsigned char calcParity(unsigned char identifier);

    [[nodiscard]] int readWithTimeout(unsigned int &remainingTime);

    /**
     * Calculates the complemented checksum for a sequence of bytes.
     * @param data Bytes to include in the checksum.
     * @param sum Initial checksum sum.
     * @return The complemented checksum.
     */
    template <unsigned int N> [[nodiscard]] unsigned char calcChecksum(const unsigned char (&data)[N], unsigned int sum)
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

    template <unsigned int N> bool sendRequest(const unsigned char (&packet)[N])
    {
        send(linDiagnosticRequestId, packet);
        unsigned char response[N]{};
        return request(linDiagnosticResponseId, response);
    }

public:
    enum class Command : unsigned char
    {
        FINISH = 0x84U,
        LOWER = 0x85U,
        RAISE = 0x86U,
        OK = 0x87U,
        CALIBRATE_END = 0xBCU,
        CALIBRATE_BEGIN = 0xBDU,
        PRE_MOVE = 0xC4U,
        IDLE = 0xFCU,
    };

    [[nodiscard]] bool begin();

    void send(unsigned char identifier);

    /**
     * Sends a desk command with a target encoder position.
     *
     * @param command Command to send.
     * @param position Encoder position associated with the command.
     */
    void sendCommand(Command command, unsigned int position);

    /**
     * Sends a LIN frame containing the specified payload.
     *
     * @param identifier LIN frame identifier.
     * @param data Payload bytes to transmit.
     */
    template <unsigned int N> void send(unsigned char identifier, const unsigned char (&data)[N])
    {
        static_assert(N <= 8U);
        const unsigned char addressByte{static_cast<unsigned char>((identifier & 0x3FU) | calcParity(identifier))};
        serialBreak();
        Serial.write(linSyncByte);
        Serial.write(addressByte);
        Serial.write(data, N);
        Serial.write(calcChecksum(data, identifier == linDiagnosticRequestId ? 0U : addressByte));
        Serial.flush();
    }

    /**
     * Receives a LIN response for the specified identifier.
     *
     * @param identifier LIN frame identifier to request.
     * @param data Buffer to populate with the received payload.
     * @return `true` if a complete response with a valid checksum is received, `false` on timeout or checksum
     * failure.
     */
    template <unsigned int N> [[nodiscard]] bool request(unsigned char identifier, unsigned char (&data)[N])
    {
        static_assert(N <= 8U);
        const unsigned char idByte{static_cast<unsigned char>((identifier & 0x3FU) | calcParity(identifier))};
        serialBreak();
        Serial.write(linSyncByte);
        Serial.write(idByte);
        Serial.flush();
        int receivedByte{};
        unsigned int remainingTime{static_cast<unsigned int>(LinFrame::frameBits * 1'000'000UL / baudRate)};
        do // NOLINT(cppcoreguidelines-avoid-do-while)
        {
            receivedByte = readWithTimeout(remainingTime);
        } while (receivedByte != -1 && receivedByte != static_cast<int>(linSyncByte));
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
        return receivedByte != -1 &&
               calcChecksum(data, identifier == linDiagnosticResponseId ? 0U : idByte) == receivedByte;
    }
};

#endif // ARDUINO_ARCH_AVR
