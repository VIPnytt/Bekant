#ifdef ARDUINO_ARCH_AVR

#include "avr/ConsoleHandler.h"

#include "avr/ControllerService.h"
#include "avr/constants.h"

/**
 * @brief Buffers a serial command and parses it when its complete payload is received.
 *
 * The first byte specifies the payload length and command identifier. USART receive errors are reported before the
 * byte is consumed.
 */
void ConsoleHandler::handle()
{
    if (Serial1.available() != 0)
    {
        const unsigned char errors{UCSR1A}; // NOLINT(clang-analyzer-core.FixedAddressDereference)
        if ((errors & ((0b1U << UPE1) | (0b1U << DOR1) | (0b1U << FE1))) != 0U)
        {
            send(State::CONSOLE, errors);
        }
        const int byte{Serial1.read()};
        if (lengthRx == 0U)
        {
            lengthRx = static_cast<unsigned char>(static_cast<unsigned char>(byte) >> 4U);
            commandRx = static_cast<Command>(static_cast<unsigned char>(byte) & 0xFU);
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
    if (commandRx == Command::CALIBRATE && lengthRx == 0U)
    {
        controller.recalibrate();
    }
    else if (commandRx == Command::POSITION && lengthRx == 2U)
    {
        const uint16_t target{static_cast<unsigned int>(bufferRx[1U]) | static_cast<unsigned int>(bufferRx[2U]) << 8U};
        if (target <= Encoder::maxLimit && target >= Encoder::minLimit)
        {
            controller.setTarget(target);
        }
    }
    else if (commandRx == Command::PRESET_HIGH && lengthRx == 0U)
    {
        controller.setTarget(controller.getPresetHigh());
    }
    else if (commandRx == Command::PRESET_HIGH && lengthRx == 2U)
    {
        controller.setPresetHigh(static_cast<unsigned int>(bufferRx[1U]) | static_cast<unsigned int>(bufferRx[2U])
                                                                               << 8U);
    }
    else if (commandRx == Command::PRESET_LOW && lengthRx == 0U)
    {
        controller.setTarget(controller.getPresetLow());
    }
    else if (commandRx == Command::PRESET_LOW && lengthRx == 2U)
    {
        controller.setPresetLow(static_cast<unsigned int>(bufferRx[1U]) | static_cast<unsigned int>(bufferRx[2U])
                                                                              << 8U);
    }
    else if (commandRx == Command::TONE && lengthRx == 2U)
    {
        controller.tone(static_cast<unsigned int>(bufferRx[1U]) | static_cast<unsigned int>(bufferRx[2U]) << 8U);
    }
}

/**
 * @brief Sends a command without a payload over Serial1.
 *
 * @param state Command to send.
 */
void ConsoleHandler::send(State state) { Serial1.write(static_cast<unsigned char>(state)); }

/**
 * @brief Writes a command with one payload byte to Serial1.
 *
 * @param state Command identifier.
 * @param byte Payload byte.
 */
void ConsoleHandler::send(State state, unsigned char byte)
{
    Serial1.write(static_cast<unsigned char>((1U << 4U) | static_cast<unsigned char>(state)));
    Serial1.write(byte);
}

/**
 * @brief Sends a command with a 16-bit unsigned value.
 *
 * @param state Command to send.
 * @param value Value to encode and send.
 */
void ConsoleHandler::send(State state, unsigned int value)
{
    const unsigned char data[2U]{
        static_cast<unsigned char>(value),
        static_cast<unsigned char>(value >> 8U),
    };
    send(state, data);
}

#endif // ARDUINO_ARCH_AVR
