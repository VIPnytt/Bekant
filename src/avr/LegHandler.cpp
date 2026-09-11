#ifdef __AVR__

#include "avr/LegHandler.h"

#include "avr/ConsoleHandler.h"

#ifdef __AVR_ATtiny841__
#include <wiring.h>
#endif // __AVR_ATtiny841__

/**
 * @brief Initializes the LIN interface and configures the connected device.
 *
 * @return `0` on success; bits 0 and 1 report no response and checksum mismatch for probe A, and bits 2 and 3
 * report the same conditions for probe B.
 */
unsigned char LegHandler::begin()
{
    pinMode(pin, OUTPUT);

#ifdef __AVR_ATtiny841__
    Serial.begin(baudRate);
#elif defined(__AVR_ATtiny1624__)
    Serial1.begin(baudRate);
#endif // __AVR_ATtiny841__
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

/**
 * @brief Requests a desk-leg status frame and validates its checksum.
 *
 * @param pid Protected identifier of the leg status frame.
 * @param node Buffer for the two-byte encoder position and one-byte state.
 * @return `0` for a valid response, bit 0 for an incomplete response, or bit 1 for a checksum mismatch.
 */
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
 * Reports a nonzero parity, data-overrun, or frame-error combination to the console before returning the byte unless
 * it matches the last reported combination.
 *
 * @param remainingTime Maximum wait time in microseconds; reduced by the time spent waiting.
 * @return int The received byte, or -1 if no byte is available before the timeout.
 */
int LegHandler::read(unsigned int &remainingTime)
{
    constexpr unsigned int interval{static_cast<unsigned int>(1'000'000UL / baudRate)};
#ifdef __AVR_ATtiny841__
    while (remainingTime != 0U && Serial.available() == 0)
#elif defined(__AVR_ATtiny1624__)
    while (remainingTime != 0U && Serial1.available() == 0)
#endif // __AVR_ATtiny841__
    {
        const unsigned int delayTime{remainingTime >= interval ? interval : remainingTime};
        delayMicroseconds(delayTime);
        remainingTime -= delayTime;
    }
#ifdef __AVR_ATtiny841__
    if (Serial.available() == 0)
#elif defined(__AVR_ATtiny1624__)
    if (Serial1.available() == 0)
#endif // __AVR_ATtiny841__
    {
        return -1;
    }
#ifdef __AVR_ATtiny841__
    // NOLINTNEXTLINE(clang-analyzer-core.FixedAddressDereference)
    const unsigned char _errors{static_cast<unsigned char>(static_cast<unsigned char>(UCSR0A >> 2U) & 0b111U)};
#elif defined(__AVR_ATtiny1624__)
    const unsigned char _errors{static_cast<unsigned char>(
        // NOLINTNEXTLINE(clang-analyzer-core.FixedAddressDereference)
        static_cast<unsigned char>(static_cast<unsigned char>(USART1.RXDATAH & USART_PERR_bm) >> 1U) |
        // NOLINTNEXTLINE(clang-analyzer-core.FixedAddressDereference)
        static_cast<unsigned char>(static_cast<unsigned char>(USART1.RXDATAH & USART_BUFOVF_bm) >> 5U) |
        // NOLINTNEXTLINE(clang-analyzer-core.FixedAddressDereference)
        static_cast<unsigned char>(USART1.RXDATAH & USART_FERR_bm))};
#endif // __AVR_ATtiny841__
    if (_errors != 0U && _errors != errors)
    {
        errors = _errors;
        console.send(ConsoleHandler::State::LIN, errors);
    }
#ifdef __AVR_ATtiny841__
    return Serial.read();
#elif defined(__AVR_ATtiny1624__)
    return Serial1.read();
#endif // __AVR_ATtiny841__
}

/**
 * @brief Requests and discards a diagnostic response within one frame time budget.
 */
void LegHandler::requestDiscardResponse()
{
    serialBreak();
#ifdef __AVR_ATtiny841__
    Serial.write(linSyncByte);
    Serial.write(getPid(linDiagnosticResponseId));
    Serial.flush();
#elif defined(__AVR_ATtiny1624__)
    Serial1.write(linSyncByte);
    Serial1.write(getPid(linDiagnosticResponseId));
    Serial1.flush();
#endif // __AVR_ATtiny841__

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

/**
 * @brief Sends a LIN response header followed by the complemented protected identifier.
 *
 * @param pid Protected identifier to transmit.
 */
void LegHandler::sendResponse(unsigned char pid)
{
    serialBreak();
#ifdef __AVR_ATtiny841__
    Serial.write(linSyncByte);
    Serial.write(pid);
    Serial.write(static_cast<unsigned char>(~pid));
    Serial.flush();
#elif defined(__AVR_ATtiny1624__)
    Serial1.write(linSyncByte);
    Serial1.write(pid);
    Serial1.write(static_cast<unsigned char>(~pid));
    Serial1.flush();
#endif // __AVR_ATtiny841__
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
#ifdef __AVR_ATtiny841__
    Serial.end();
#elif defined(__AVR_ATtiny1624__)
    Serial1.end();
#endif // __AVR_ATtiny841__
    pinMode(pin, OUTPUT);
    digitalWrite(pin, LOW);
    delayMicroseconds(static_cast<unsigned int>((LinFrame::breakBits + 2UL) * 1'000'000UL / baudRate)); // ~780 µs
    digitalWrite(pin, HIGH);
    delayMicroseconds(static_cast<unsigned int>(LinFrame::delimiterBits * 1'000'000UL / baudRate));
#ifdef __AVR_ATtiny841__
    Serial.begin(baudRate);
#elif defined(__AVR_ATtiny1624__)
    Serial1.begin(baudRate);
#endif // __AVR_ATtiny841__
}

#endif // __AVR__
