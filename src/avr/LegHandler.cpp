#ifdef ARDUINO_ARCH_AVR

#include "avr/LegHandler.h"

#include "avr/ConsoleHandler.h"
#include "avr/constants.h"

#include <wiring.h>

/**
 * @brief Initializes the LIN interface and configures the connected device.
 *
 * @return `0` on success; bits 0 and 1 report no response and checksum mismatch for probe A, and bits 2 and 3
 * report the same conditions for probe B.
 */
unsigned char LegHandler::begin()
{
    pinMode(Pin::deskRx, INPUT);
    pinMode(Pin::deskTx, OUTPUT);
    digitalWrite(Pin::deskTx, HIGH);
    TCCR1A = 0U;
    TCCR1B = 0b1U << CS11;
    TCCR1C = 0U;
    TCNT1 = 0U;
    TIMSK1 = 0U;
    static constexpr uint16_t ubrrValue{
        static_cast<uint16_t>((static_cast<unsigned long>(F_CPU) / (16UL * baudRate)) - 1UL)};
    UBRR0H = static_cast<uint8_t>(ubrrValue >> 8U);
    UBRR0L = static_cast<uint8_t>(ubrrValue & 0xFFU);
    UCSR0A = 0U;
    UCSR0B = (0b1U << RXEN0) | (0b1U << TXEN0);
    UCSR0C = (0b1U << UCSZ01) | (0b1U << UCSZ00);
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
 * @brief Sends a single byte over USART0 directly to the hardware data register.
 *
 * @param byte Data byte to transmit.
 */
void LegHandler::write(uint8_t byte)
{
    while ((UCSR0A & (0b1U << UDRE0)) == 0U)
    {
        yield();
    }
    UCSR0A |= 0b1U << TXC0;
    UDR0 = byte;
}

/**
 * @brief Waits until the final stop bit has completely cleared the USART0 transmit shift register.
 */
void LegHandler::flushTx()
{
    while ((UCSR0A & (0b1U << TXC0)) == 0U)
    {
        yield();
    }
}

/**
 * @brief Drains any residual bytes or framing errors from the USART0 receive FIFO.
 */
void LegHandler::flushRx()
{
    while ((UCSR0A & (0b1U << RXC0)) != 0U)
    {
        static_cast<void>(UDR0);
    }
}

/**
 * @brief Reads a serial byte within the remaining Timer1 time budget.
 *
 * Reports a nonzero parity, data-overrun, or frame-error combination to the console unless it matches the last
 * reported combination.
 *
 * @param remainingTime Remaining Timer1 ticks, reduced by the elapsed ticks down to zero. One tick is one microsecond
 * at an 8 MHz CPU clock.
 * @return int The received byte, or -1 if the budget is initially zero or expires while waiting for a byte.
 */
int LegHandler::read(unsigned int &remainingTime)
{
    if (remainingTime == 0U)
    {
        return -1;
    }
    const uint16_t start{TCNT1};
    while ((UCSR0A & (0b1U << RXC0)) == 0U)
    {
        const uint16_t elapsed{static_cast<uint16_t>(TCNT1 - start)};
        if (elapsed >= remainingTime)
        {
            remainingTime = 0U;
            return -1;
        }
    }
    const uint16_t elapsed{static_cast<uint16_t>(TCNT1 - start)};
    if (elapsed >= remainingTime)
    {
        remainingTime = 0U;
    }
    else
    {
        remainingTime -= elapsed;
    }
    const unsigned char _errors{static_cast<unsigned char>((static_cast<unsigned int>(UCSR0A) >> 2U) & 0b111U)};
    if (_errors != 0U && _errors != errors)
    {
        errors = _errors;
        ConsoleHandler::send(ConsoleHandler::State::LIN, errors);
    }
    return static_cast<int>(UDR0);
}

/**
 * @brief Requests and discards a diagnostic response within one frame time budget.
 */
void LegHandler::requestDiscardResponse()
{
    serialBreak();
    write(linSyncByte);
    write(getPid(linDiagnosticResponseId));
    flushTx();
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
    write(linSyncByte);
    write(pid);
    write(static_cast<unsigned char>(~pid));
    flushTx();
}

/**
 * @brief Generates a LIN break and delimiter, temporarily disabling USART0 transmission and reception.
 *
 * Discards any bytes left in the receive buffer after re-enabling USART0.
 */
void LegHandler::serialBreak()
{
    UCSR0B &= ~((0b1U << TXEN0) | (0b1U << RXEN0));
    digitalWrite(Pin::deskTx, LOW);
    delayMicroseconds(static_cast<unsigned int>((LinFrame::breakBits + 2UL) * 1'000'000UL / baudRate)); // ~781 us
    digitalWrite(Pin::deskTx, HIGH);
    delayMicroseconds(static_cast<unsigned int>(LinFrame::delimiterBits * 1'000'000UL / baudRate)); // ~52 us
    UCSR0B |= (0b1U << TXEN0) | (0b1U << RXEN0);
    flushRx();
}

#endif // ARDUINO_ARCH_AVR
