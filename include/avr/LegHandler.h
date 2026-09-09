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

    void requestDiscardResponse();

    void sendResponse(unsigned char pid);

    void serialBreak();

    [[nodiscard]] int read(unsigned int &remainingTime);

public:
    [[nodiscard]] static constexpr unsigned char getPid(unsigned char identifier)
    {
        identifier &= 0x3FU;
        const unsigned int parity0{
            static_cast<unsigned int>(identifier & 1U) ^ (static_cast<unsigned int>(identifier >> 1U) & 1U) ^
            (static_cast<unsigned int>(identifier >> 2U) & 1U) ^ (static_cast<unsigned int>(identifier >> 4U) & 1U)};
        const unsigned int parity1{
            ~((static_cast<unsigned int>(identifier >> 1U) & 1U) ^ (static_cast<unsigned int>(identifier >> 3U) & 1U) ^
              (static_cast<unsigned int>(identifier >> 4U) & 1U) ^ (static_cast<unsigned int>(identifier >> 5U) & 1U)) &
            1U};
        return static_cast<unsigned char>(identifier | ((parity0 | (parity1 << 1U)) << 6U));
    }

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

    [[nodiscard]] unsigned char begin();

    [[nodiscard]] unsigned char getLeg(unsigned char pid, unsigned char (&node)[3U]);

    /**
     * Sends a desk command with a target encoder position.
     *
     * @param command Command to send.
     * @param position Encoder position associated with the command.
     */
    void sendCommand(Command command, unsigned int position);

    /**
     * Calculates the complemented checksum for a sequence of bytes.
     * @param data Bytes to include in the checksum.
     * @param sum Initial checksum sum.
     * @return The complemented checksum.
     */
    template <unsigned int N>
    [[nodiscard]] unsigned char getChecksum(const unsigned char (&data)[N], unsigned int sum = 0U)
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

    /**
     * Receives a LIN response for the specified identifier.
     *
     * @param identifier LIN frame identifier to request.
     * @param data Buffer to populate with the received payload.
     * @return `true` if a complete response with a valid checksum is received, `false` on timeout or checksum
     * failure.
     */
    template <unsigned int N> int receiveResponse(unsigned char pid, unsigned char (&data)[N])
    {
        static_assert(N <= 8U);
        serialBreak();
        Serial.write(linSyncByte);
        Serial.write(pid);
        Serial.flush();
        int receivedByte{};
        unsigned int remainingTime{static_cast<unsigned int>(LinFrame::frameBits * 1'000'000UL / baudRate)};
        do // NOLINT(cppcoreguidelines-avoid-do-while)
        {
            receivedByte = read(remainingTime);
        } while (receivedByte != -1 && receivedByte != static_cast<int>(linSyncByte));
        if (receivedByte == -1)
        {
            return -1;
        }
        do // NOLINT(cppcoreguidelines-avoid-do-while)
        {
            receivedByte = read(remainingTime);
        } while (receivedByte != -1 && receivedByte != pid);
        if (receivedByte == -1)
        {
            return -1;
        }
        for (unsigned char &dataByte : data)
        {
            receivedByte = read(remainingTime);
            if (receivedByte == -1)
            {
                return -1;
            }
            dataByte = static_cast<unsigned char>(receivedByte);
        }
        return read(remainingTime);
    }

    template <unsigned int N> void sendDiagnosticRequest(const unsigned char (&data)[N])
    {
        static_assert(N <= 8U);
        serialBreak();
        Serial.write(linSyncByte);
        Serial.write(getPid(linDiagnosticRequestId));
        Serial.write(data, N);
        Serial.write(getChecksum(data));
        Serial.flush();
    }

    template <unsigned int N> void sendResponse(unsigned char pid, const unsigned char (&data)[N])
    {
        static_assert(N <= 8U);
        serialBreak();
        Serial.write(linSyncByte);
        Serial.write(pid);
        Serial.write(data, N);
        Serial.write(getChecksum(data, pid));
        Serial.flush();
    }
};

#endif // ARDUINO_ARCH_AVR
