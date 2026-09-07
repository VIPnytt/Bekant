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
        if (commandLength == 0U)
        {
            commandLength = static_cast<unsigned char>(static_cast<unsigned char>(byte) >> 4U);
            command = static_cast<Command>(static_cast<unsigned char>(byte) & 0x0FU);
            commandBytes = 0U;
        }
        commandBuffer[commandBytes++] = static_cast<unsigned char>(byte);
        if (commandBytes == commandLength + 1U)
        {
            parse();
            commandLength = 0U;
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
    if (command == Command::CALIBRATE && commandLength == 0U)
    {
        desk.recalibrate();
    }
    else if (command == Command::POSITION && commandLength == 2U)
    {
        const uint16_t target{static_cast<unsigned int>(commandBuffer[1U]) |
                              static_cast<unsigned int>(commandBuffer[2U]) << 8U};
        if (target <= Encoder::maxLimit && target >= Encoder::minLimit)
        {
            desk.setTarget(target);
        }
    }
    else if (command == Command::PRESET_HIGH && commandLength == 0U)
    {
        desk.setTarget(desk.getPresetHigh());
    }
    else if (command == Command::PRESET_HIGH && commandLength == 2U)
    {
        desk.setPresetHigh(static_cast<unsigned int>(commandBuffer[1U]) | static_cast<unsigned int>(commandBuffer[2U])
                                                                              << 8U);
    }
    else if (command == Command::PRESET_LOW && commandLength == 0U)
    {
        desk.setTarget(desk.getPresetLow());
    }
    else if (command == Command::PRESET_LOW && commandLength == 2U)
    {
        desk.setPresetLow(static_cast<unsigned int>(commandBuffer[1U]) | static_cast<unsigned int>(commandBuffer[2U])
                                                                             << 8U);
    }
    else if (command == Command::TONE && commandLength == 2U)
    {
        desk.tone(static_cast<unsigned int>(commandBuffer[1U]) | static_cast<unsigned int>(commandBuffer[2U]) << 8U);
    }
}

/**
 * @brief Sends a command with a 16-bit unsigned value.
 *
 * @param state Command to send.
 * @param value Value to encode and send.
 */
void ConsoleHandler::print(State state, unsigned int value)
{
    write(state, static_cast<unsigned char>(value & 0xFFU), static_cast<unsigned char>(value >> 8U));
}

/**
 * @brief Sends a command without a payload over Serial1.
 *
 * @param state Command to send.
 */
void ConsoleHandler::write(State state) { Serial1.write(static_cast<unsigned char>(state)); }

/**
 * @brief Writes a command with one payload byte to Serial1.
 *
 * @param state Command identifier.
 * @param byte Payload byte.
 */
void ConsoleHandler::write(State state, unsigned char byte)
{
    Serial1.write((1U << 4U) | static_cast<unsigned char>(state));
    Serial1.write(byte);
}

/**
 * @brief Writes a command with a two-byte payload to Serial1.
 *
 * @param state Command identifier.
 * @param byte1 First payload byte.
 * @param byte2 Second payload byte.
 */
void ConsoleHandler::write(State state, unsigned char byte1, unsigned char byte2)
{
    Serial1.write((2U << 4U) | static_cast<unsigned char>(state));
    Serial1.write(byte1);
    Serial1.write(byte2);
}

/**
 * @brief Writes a command with a three-byte payload to Serial1.
 *
 * @param state Command identifier.
 * @param byte1 First payload byte.
 * @param byte2 Second payload byte.
 * @param byte3 Third payload byte.
 */
void ConsoleHandler::write(State state, unsigned char byte1, unsigned char byte2, unsigned char byte3)
{
    Serial1.write((3U << 4U) | static_cast<unsigned char>(state));
    Serial1.write(byte1);
    Serial1.write(byte2);
    Serial1.write(byte3);
}

#endif // ARDUINO_ARCH_AVR
