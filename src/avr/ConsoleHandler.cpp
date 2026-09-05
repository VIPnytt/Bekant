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
 * @brief Processes the buffered serial command and applies the requested action.
 *
 * Supports recalibration, movement targets, high and low preset recall or updates,
 * and tone frequency commands.
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
        else if (buffer[0U] == 'h' || buffer[0U] == 'l' || buffer[0U] == 'p' || buffer[0U] == 't')
        {
            send(static_cast<char>(buffer[0U] - ' '), value);
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

void ConsoleHandler::send(char command, unsigned int value)
{
    Serial1.write(static_cast<int>(command));
    Serial1.print(value);
    Serial1.write(static_cast<int>('\n'));
}

#endif // ARDUINO_ARCH_AVR
