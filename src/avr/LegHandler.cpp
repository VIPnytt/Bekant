#ifdef ARDUINO_ARCH_AVR

#include "avr/LegHandler.h"

#include "avr/ConsoleHandler.h"
#include "avr/constants.h"

#include <wiring.h>

/**
 * @brief Initializes the LIN interface and configures the connected device.
 *
 * @return true if initialization and device detection succeed, false otherwise.
 */
unsigned char LegHandler::begin()
{
    pinMode(Pin::lin, OUTPUT);
    Serial.begin(baudRate);
    constexpr unsigned char initial[3U][2U]{
        {0x7U, 0xFFU},
        {0x7U, 0xFFU},
        {0x1U, 0x7U},
    };
    for (const unsigned char (&data)[2U] : initial)
    {
        const unsigned char packet[8U]{0xFFU, data[0U], data[1U], 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU};
        sendDiagnosticRequest(packet);
    }
    const unsigned char packet[8U]{0xD0U, 0x2U, 0x7U, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU};
    sendDiagnosticRequest(packet);
    requestDiscardResponse();
    unsigned char pid{0U};
    for (; pid < 8U; ++pid)
    {
        const unsigned char probeA[8U]{pid, 0x2U, 0x7U, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU};
        sendDiagnosticRequest(probeA);
        unsigned char response[sizeof(probeA)]{};
        const int checksum{receiveResponse(getPid(linDiagnosticResponseId), response)};
        if (checksum != -1 && getChecksum(response) == checksum)
        {
            break;
        }
        if (pid == 7U)
        {
            return static_cast<unsigned char>(checksum == -1 ? 0b1U : 0b1U << 1U);
        }
    }
    constexpr unsigned char preProbe[6U][2U]{
        {0x6U, 0x9U},
        {0x6U, 0xCU},
        {0x6U, 0xDU},
        {0x6U, 0xAU},
        {0x6U, 0xBU},
        {0x4U, 0x0U},
    };
    for (const unsigned char (&data)[2U] : preProbe)
    {
        const unsigned char packet[8U]{pid, data[0U], data[1U], 0x0U, 0xFFU, 0xFFU, 0xFFU, 0xFFU};
        sendDiagnosticRequest(packet);
        requestDiscardResponse();
    }
    for (; pid < 8U; ++pid)
    {
        const unsigned char probeB[8U]{pid, 0x2U, 0x0U, 0x0U, 0xFFU, 0xFFU, 0xFFU, 0xFFU};
        // unsigned char response[sizeof(probeB)]{};
        sendDiagnosticRequest(probeB);
        unsigned char response[sizeof(probeB)]{};
        const int checksum{receiveResponse(getPid(linDiagnosticResponseId), response)};
        if (checksum != -1 && getChecksum(response) == checksum)
        {
            break;
        }
        if (pid == 7U)
        {
            return static_cast<unsigned char>(checksum == -1 ? 0b1U << 2U : 0b1U << 3U);
        }
    }
    constexpr unsigned char preBroadcast[6U][2U]{
        {0x6U, 0x9U},
        {0x6U, 0xCU},
        {0x6U, 0xDU},
        {0x6U, 0xAU},
        {0x6U, 0xBU},
        {0x4U, 0x1U},
    };
    for (const unsigned char (&data)[2U] : preBroadcast)
    {
        const unsigned char packet[8U]{pid, data[0U], data[1U], 0x0U, 0xFFU, 0xFFU, 0xFFU, 0xFFU};
        sendDiagnosticRequest(packet);
        requestDiscardResponse();
    }
    for (; pid < 8U; ++pid)
    {
        const unsigned char broadcast[8U]{pid, 0x2U, 0x1U, 0x0U, 0xFFU, 0xFFU, 0xFFU, 0xFFU};
        sendDiagnosticRequest(broadcast);
        requestDiscardResponse();
    }
    constexpr unsigned char postBroadcast[2U]{0x1U, 0x2U};
    for (const unsigned char &data : postBroadcast)
    {
        const unsigned char packet[8U]{0xD0U, data, 0x7U, 0x0U, 0xFFU, 0xFFU, 0xFFU, 0xFFU};
        sendDiagnosticRequest(packet);
    }
    constexpr unsigned char final[3U]{0xF6U, 0xFFU, 0xBFU};
    sendResponse(getPid(0x12U), final);
    return 0U;
}

unsigned char LegHandler::getLeg(unsigned char pid, unsigned char (&node)[3U])
{
    const int checksum{receiveResponse(pid, node)};
    if (checksum == -1)
    {
        return 0b1U;
    }
    if (getChecksum(node, pid) == checksum)
    {
        return 0U;
    }
    return 0b1U << 1U;
}

/**
 * @brief Reads a serial byte within the available time budget.
 *
 * @param remainingTime Maximum wait time in microseconds; reduced by the time spent waiting.
 * @return int The received byte, or -1 if no byte is available before the timeout.
 */
int LegHandler::read(unsigned int &remainingTime)
{
    constexpr unsigned int interval{static_cast<unsigned int>(1'000'000UL / baudRate)};
    while (remainingTime != 0U && Serial.available() == 0)
    {
        const unsigned int delayTime{remainingTime >= interval ? interval : remainingTime};
        delayMicroseconds(delayTime);
        remainingTime -= delayTime;
    }
    if (Serial.available() == 0)
    {
        return -1;
    }
    const unsigned char errors{UCSR0A}; // NOLINT(clang-analyzer-core.FixedAddressDereference)
    if ((errors & ((0b1U << UPE0) | (0b1U << DOR0) | (0b1U << FE0))) != 0U)
    {
        Serial1.write((1U << 4U) | static_cast<unsigned char>(ConsoleHandler::State::LIN));
        Serial1.write(errors);
    }
    return Serial.read();
}

void LegHandler::requestDiscardResponse()
{
    serialBreak();
    Serial.write(linSyncByte);
    Serial.write(getPid(linDiagnosticResponseId));
    Serial.flush();
    unsigned int remainingTime{static_cast<unsigned int>(LinFrame::frameBits * 1'000'000UL / baudRate)};
    while (remainingTime != 0U)
    {
        static_cast<void>(read(remainingTime));
    }
}

/**
 * @brief Sends a command and position payload over the LIN interface.
 *
 * @param command Command code to transmit.
 * @param position Position value included in the command payload.
 */
void LegHandler::sendCommand(Command command, unsigned int position)
{
    for (unsigned char idx{0U}; idx < 6U; ++idx)
    {
        sendResponse(getPid(0x10U));
    }
    sendResponse(getPid(0x1U));
    const unsigned char packet[3U]{
        static_cast<unsigned char>(position & 0xFFU),
        static_cast<unsigned char>(position >> 8U),
        static_cast<unsigned char>(command),
    };
    sendResponse(getPid(0x12U), packet);
}

void LegHandler::sendResponse(unsigned char pid)
{
    serialBreak();
    Serial.write(linSyncByte);
    Serial.write(pid);
    Serial.write(static_cast<unsigned char>(~pid));
    Serial.flush();
}

/**
 * @brief Generates a LIN break and delimiter signal on the configured pin.
 *
 * Temporarily stops serial communication while driving the LIN pin low for
 * the break duration and high for the delimiter duration, then resumes
 * serial communication.
 */
void LegHandler::serialBreak()
{
    Serial.end();
    pinMode(Pin::lin, OUTPUT);
    digitalWrite(Pin::lin, LOW);
    delayMicroseconds(static_cast<unsigned int>((LinFrame::breakBits + 2UL) * 1'000'000UL / baudRate)); // ~780 µs
    digitalWrite(Pin::lin, HIGH);
    delayMicroseconds(static_cast<unsigned int>(LinFrame::delimiterBits * 1'000'000UL / baudRate));
    Serial.begin(baudRate);
}

#endif // ARDUINO_ARCH_AVR
