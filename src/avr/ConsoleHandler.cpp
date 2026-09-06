#ifdef ARDUINO_ARCH_AVR

#include "avr/ConsoleHandler.h"

#include "avr/DeskService.h"
#include "avr/constants.h"

#include <HardwareSerial.h>

/**
 * @brief Buffers serial input and parses each completed newline-terminated command.
 *
 * Discards empty lines and prevents writes beyond the command buffer capacity.
 */
void ConsoleHandler::handle()
{
    const int byte{Serial1.read()};
    if (byte == static_cast<int>('\n') && length != 0U)
    {
        if (length <= sizeof(buffer))
        {
            process();
        }
        length = 0U;
    }
    else if (byte != -1 && byte != static_cast<int>('\n'))
    {
        if (length < sizeof(buffer))
        {
            buffer[length++] = static_cast<char>(byte);
        }
        else if (length == sizeof(buffer))
        {
            ++length;
        }
    }
}

/**
 * @brief Applies the buffered serial command when it is valid.
 *
 * Numeric commands update presets, the target, or tone frequency within the
 * encoder limits. Single-character commands recalibrate or move to a stored
 * preset; unsupported or out-of-range commands are ignored.
 */
void ConsoleHandler::process()
{
    if (length >= 2U)
    {
        const unsigned int value{parseDigits()};
        if (value <= Encoder::maxLimit && value >= Encoder::minLimit)
        {
            switch (buffer[0U]) // NOLINT(bugprone-switch-missing-default-case)
            {
            case 'h':
                desk.setPresetHigh(value);
                break;
            case 'l':
                desk.setPresetLow(value);
                break;
            case 'p':
                desk.setTarget(value);
                break;
            case 't':
                desk.tone(value);
                break;
            }
        }
    }
    else
    {
        switch (buffer[0U]) // NOLINT(bugprone-switch-missing-default-case)
        {
        case 'c':
            desk.recalibrate();
            break;
        case 'h':
            desk.setTarget(desk.getPresetHigh());
            break;
        case 'l':
            desk.setTarget(desk.getPresetLow());
            break;
        }
    }
}

/**
 * @brief Parses the numeric characters following the first character in the command buffer.
 *
 * @return The parsed unsigned integer, or zero if the suffix contains a non-digit character.
 */
unsigned int ConsoleHandler::parseDigits()
{
    unsigned int value{0U};
    for (unsigned char idx{1U}; idx < length; ++idx)
    {
        if (buffer[idx] < '0' || buffer[idx] > '9')
        {
            return 0U;
        }
        value *= 10U;
        value += static_cast<unsigned int>(buffer[idx] - '0');
    }
    return value;
}

/**
 * @brief Sends a command with a 16-bit unsigned value.
 *
 * @param command Command to send.
 * @param value Value to encode and send.
 */
void ConsoleHandler::print(Command command, unsigned int value)
{
    write(command, static_cast<unsigned char>(value & 0xFFU), static_cast<unsigned char>(value >> 8U));
}

/**
 * @brief Sends a command without a payload over Serial1.
 *
 * @param command Command to send.
 */
void ConsoleHandler::write(Command command) { Serial1.write(static_cast<unsigned char>(command)); }

/**
 * @brief Writes a command with one payload byte to Serial1.
 *
 * @param command Command identifier.
 * @param byte Payload byte.
 */
void ConsoleHandler::write(Command command, unsigned char byte)
{
    Serial1.write((1U << 4U) | static_cast<unsigned char>(command));
    Serial1.write(byte);
}

/**
 * @brief Writes a command with a two-byte payload to Serial1.
 *
 * @param command Command identifier.
 * @param byte1 First payload byte.
 * @param byte2 Second payload byte.
 */
void ConsoleHandler::write(Command command, unsigned char byte1, unsigned char byte2)
{
    Serial1.write((2U << 4U) | static_cast<unsigned char>(command));
    Serial1.write(byte1);
    Serial1.write(byte2);
}

/**
 * @brief Writes a command with a three-byte payload to Serial1.
 *
 * @param command Command identifier.
 * @param byte1 First payload byte.
 * @param byte2 Second payload byte.
 * @param byte3 Third payload byte.
 */
void ConsoleHandler::write(Command command, unsigned char byte1, unsigned char byte2, unsigned char byte3)
{
    Serial1.write((3U << 4U) | static_cast<unsigned char>(command));
    Serial1.write(byte1);
    Serial1.write(byte2);
    Serial1.write(byte3);
}

#endif // ARDUINO_ARCH_AVR
