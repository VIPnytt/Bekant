#ifdef ARDUINO_ARCH_AVR

#include "avr/ConsoleHandler.h"

#include "avr/DeskService.h"

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
    if (buffer[0U] == 'c')
    {
        desk.recalibrate();
    }
    else if (buffer[0U] == 'h')
    {
        length == 1U ? desk.setTarget(desk.getPresetHigh()) : desk.setPresetHigh(parseDigits());
    }
    else if (buffer[0U] == 'l')
    {
        length == 1U ? desk.setTarget(desk.getPresetLow()) : desk.setPresetLow(parseDigits());
    }
    else if (buffer[0U] == 'p')
    {
        desk.setTarget(parseDigits());
    }
    else if (buffer[0U] == 't')
    {
        desk.tone(parseDigits());
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
    for (unsigned int idx{1U}; idx < length; ++idx)
    {
        value *= 10U;
        value += static_cast<unsigned int>(buffer[idx] - '0');
    }
    return value;
}

#endif // ARDUINO_ARCH_AVR
