#ifdef ARDUINO_ARCH_AVR

#include "avr/ConsoleHandler.h"

#include "avr/DeskService.h"
#include "avr/constants.h"

#include <HardwareSerial.h>

/**
 * @brief Buffers a serial command and parses it when its complete payload is received.
 *
 * The first byte specifies the payload length and command identifier.
 */
void ConsoleHandler::handle()
{
    const int byte{Serial1.read()};
    if (byte != -1)
    {
        if (lengthRx == 0U)
        {
            lengthRx = static_cast<unsigned char>(static_cast<unsigned char>(byte) >> 4U);
            commandRx = static_cast<unsigned char>(byte) & 0x0FU;
            bytesRx = 0U;
        }
        bufferRx[bytesRx++] = static_cast<unsigned char>(byte);
        if (bytesRx == lengthRx + 1U)
        {
            parse();
            lengthRx = 0U;
        }
    }
}

/**
 * @brief Applies the buffered command when its command and payload are valid.
 *
 * Recalibrates, updates the target or presets, or sets the tone frequency.
 * Position targets outside the encoder limits and unsupported command or payload
 * combinations are ignored.
 */
void ConsoleHandler::parse()
{
    if (commandRx == static_cast<unsigned char>(Command::CALIBRATE) && lengthRx == 0U)
    {
        desk.recalibrate();
    }
    else if (commandRx == static_cast<unsigned char>(Command::POSITION) && lengthRx == 2U)
    {
        const uint16_t target{static_cast<unsigned int>(bufferRx[1U]) | static_cast<unsigned int>(bufferRx[2U]) << 8U};
        if (target <= Encoder::maxLimit && target >= Encoder::minLimit)
        {
            desk.setTarget(target);
        }
    }
    else if (commandRx == static_cast<unsigned char>(Command::PRESET_HIGH) && lengthRx == 0U)
    {
        desk.setTarget(desk.getPresetHigh());
    }
    else if (commandRx == static_cast<unsigned char>(Command::PRESET_HIGH) && lengthRx == 2U)
    {
        desk.setPresetHigh(static_cast<unsigned int>(bufferRx[1U]) | static_cast<unsigned int>(bufferRx[2U]) << 8U);
    }
    else if (commandRx == static_cast<unsigned char>(Command::PRESET_LOW) && lengthRx == 0U)
    {
        desk.setTarget(desk.getPresetLow());
    }
    else if (commandRx == static_cast<unsigned char>(Command::PRESET_LOW) && lengthRx == 2U)
    {
        desk.setPresetLow(static_cast<unsigned int>(bufferRx[1U]) | static_cast<unsigned int>(bufferRx[2U]) << 8U);
    }
    else if (commandRx == static_cast<unsigned char>(Command::TONE) && lengthRx == 2U)
    {
        desk.tone(static_cast<unsigned int>(bufferRx[1U]) | static_cast<unsigned int>(bufferRx[2U]) << 8U);
    }
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
