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
bool LegHandler::begin()
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
        send(linDiagnosticRequestId, packet);
    }
    const unsigned char packet[8U]{0xD0U, 0x2U, 0x7U, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU};
    sendRequest(packet);
    unsigned char pid{0U};
    for (; pid < 8U; ++pid)
    {
        const unsigned char probeA[8U]{pid, 0x2U, 0x7U, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU};
        if (sendRequest(probeA))
        {
            break;
        }
    }
    if (pid == 8U)
    {
        return false;
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
        sendRequest(packet);
    }
    for (; pid < 8U; ++pid)
    {
        const unsigned char probeB[8U]{pid, 0x2U, 0x0U, 0x0U, 0xFFU, 0xFFU, 0xFFU, 0xFFU};
        if (sendRequest(probeB))
        {
            break;
        }
    }
    if (pid == 8U)
    {
        return false;
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
        sendRequest(packet);
    }
    for (; pid < 8U; ++pid)
    {
        const unsigned char broadcast[8U]{pid, 0x2U, 0x1U, 0x0U, 0xFFU, 0xFFU, 0xFFU, 0xFFU};
        sendRequest(broadcast);
    }
    constexpr unsigned char postBroadcast[2U]{0x1U, 0x2U};
    for (const unsigned char &data : postBroadcast)
    {
        const unsigned char packet[8U]{0xD0U, data, 0x7U, 0x0U, 0xFFU, 0xFFU, 0xFFU, 0xFFU};
        send(linDiagnosticRequestId, packet);
    }
    constexpr unsigned char final[3U]{0xF6U, 0xFFU, 0xBFU};
    send(0x12U, final);
    return true;
}

/**
 * @brief Calculates the LIN protected identifier parity bits.
 *
 * @param identifier Six-bit LIN identifier.
 * @return Parity bits positioned in bits 6 and 7.
 */
unsigned char LegHandler::calcParity(unsigned char identifier)
{
    const unsigned int parity0{
        static_cast<unsigned int>(identifier & 1U) ^ (static_cast<unsigned int>(identifier >> 1U) & 1U) ^
        (static_cast<unsigned int>(identifier >> 2U) & 1U) ^ (static_cast<unsigned int>(identifier >> 4U) & 1U)};
    const unsigned int parity1{
        ~((static_cast<unsigned int>(identifier >> 1U) & 1U) ^ (static_cast<unsigned int>(identifier >> 3U) & 1U) ^
          (static_cast<unsigned int>(identifier >> 4U) & 1U) ^ (static_cast<unsigned int>(identifier >> 5U) & 1U)) &
        1U};
    return static_cast<unsigned char>((parity0 | (parity1 << 1U)) << 6U);
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
    if ((errors & ((0b1U << DOR0) | (0b1U << FE0))) != 0U)
    {
        Serial1.write((1U << 4U) | static_cast<unsigned char>(ConsoleHandler::State::LIN));
        Serial1.write(errors);
    }
    return Serial.read();
}

/**
 * @brief Transmits a LIN frame for the specified identifier.
 *
 * @param identifier LIN identifier; its lower six bits are used to form the protected identifier.
 */
void LegHandler::send(unsigned char identifier)
{
    const unsigned char address{static_cast<unsigned char>((identifier & 0x3FU) | calcParity(identifier))};
    serialBreak();
    Serial.write(linSyncByte);
    Serial.write(address);
    Serial.write(identifier == linDiagnosticRequestId ? 0xFFU : static_cast<unsigned char>(~address));
    Serial.flush();
}

/**
 * @brief Sends a command and position payload over the LIN interface.
 *
 * @param command Command code to transmit.
 * @param position Position value included in the command payload.
 */
void LegHandler::sendCommand(LegHandler::Command command, unsigned int position)
{
    for (unsigned char idx{0U}; idx < 6U; ++idx)
    {
        send(0x10U);
    }
    send(0x1U);
    const unsigned char packet[3U]{
        static_cast<unsigned char>(position & 0xFFU),
        static_cast<unsigned char>(position >> 8U),
        static_cast<unsigned char>(command),
    };
    send(0x12U, packet);
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
