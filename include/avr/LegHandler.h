#pragma once

#ifdef __AVR__

#include <HardwareSerial.h>

#ifdef __AVR_ATtiny841__
#include <core_pins.h>
#elif defined(__AVR_ATtiny1624__)
#include <pins_arduino.h>
#endif // __AVR_ATtiny841__

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
    static constexpr unsigned char pin{PIN_PA1};

    unsigned char errors{0U};

    HardwareSerial *const lin;

    void requestDiscardResponse();

    void sendResponse(unsigned char pid);

    void serialBreak();

    [[nodiscard]] int read(unsigned int &remainingTime);

public:
#ifdef __AVR_ATtiny841__
    LegHandler() : lin{&Serial} {}
#elif defined(__AVR_ATtiny1624__)
    LegHandler() : lin{&Serial1} {}
#endif // __AVR_ATtiny841__

    static constexpr unsigned char maxDelta{0xFFU};

    static constexpr unsigned char minLimit{0xFFU};

    static constexpr unsigned char targetOffset{137U};

    static constexpr unsigned int maxLimit{0b1U << 13U};

    /**
     * Builds a LIN protected identifier from a frame identifier.
     *
     * @param identifier Frame identifier; only the lower six bits are used.
     * @return The identifier with its parity bits in the upper two bits.
     */
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
     * Requests a LIN response and reads its payload and checksum.
     *
     * @param pid Protected identifier to transmit in the response header.
     * @param data Buffer to populate with the received payload.
     * @return The received checksum byte, or `-1` if the response is incomplete.
     */
    template <unsigned int N> int receiveResponse(unsigned char pid, unsigned char (&data)[N])
    {
        static_assert(N <= 8U);
        serialBreak();
        lin->write(linSyncByte);
        lin->write(pid);
        lin->flush();
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

    /**
     * Sends a LIN diagnostic request with a classic checksum.
     *
     * @param data Diagnostic payload to transmit.
     */
    template <unsigned int N> void sendDiagnosticRequest(const unsigned char (&data)[N])
    {
        static_assert(N <= 8U);
        serialBreak();
        lin->write(linSyncByte);
        lin->write(getPid(linDiagnosticRequestId));
        lin->write(data, N);
        lin->write(getChecksum(data));
        lin->flush();
    }

    /**
     * Sends a LIN response with an enhanced checksum.
     *
     * @param pid Protected identifier to transmit.
     * @param data Response payload to transmit.
     */
    template <unsigned int N> void sendResponse(unsigned char pid, const unsigned char (&data)[N])
    {
        static_assert(N <= 8U);
        serialBreak();
        lin->write(linSyncByte);
        lin->write(pid);
        lin->write(data, N);
        lin->write(getChecksum(data, pid));
        lin->flush();
    }
};

#endif // __AVR__
