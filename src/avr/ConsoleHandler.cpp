#ifdef __AVR__

#include "avr/ConsoleHandler.h"

#include "avr/ControllerService.h"
#include "avr/ToneHandler.h"

#ifdef __AVR_ATtiny841__
#define esp Serial1
#elif defined(__AVR_ATtiny1624__)
#define esp Serial
#endif // __AVR_ATtiny841__

void ConsoleHandler::begin() { esp.begin(115'200UL); }

/**
 * @brief Buffers a serial command and parses it when its complete payload is received.
 *
 * The first byte specifies the payload length and command identifier. A nonzero USART receive-error combination is
 * reported before the byte is consumed unless it matches the last reported combination.
 */
void ConsoleHandler::handle()
{
    if (esp.available() != 0)
    {
#ifdef __AVR_ATtiny841__
        // NOLINTNEXTLINE(clang-analyzer-core.FixedAddressDereference)
        const unsigned char _errors{static_cast<unsigned char>(static_cast<unsigned char>(UCSR1A >> 2U) & 0b111U)};
#elif defined(__AVR_ATtiny1624__)
        const unsigned char _errors{static_cast<unsigned char>(
            // NOLINTNEXTLINE(clang-analyzer-core.FixedAddressDereference)
            static_cast<unsigned char>(static_cast<unsigned char>(USART0.RXDATAH & USART_PERR_bm) >> 1U) |
            // NOLINTNEXTLINE(clang-analyzer-core.FixedAddressDereference)
            static_cast<unsigned char>(static_cast<unsigned char>(USART0.RXDATAH & USART_BUFOVF_bm) >> 5U) |
            // NOLINTNEXTLINE(clang-analyzer-core.FixedAddressDereference)
            static_cast<unsigned char>(USART0.RXDATAH & USART_FERR_bm))};
#endif // __AVR_ATtiny841__
        if (_errors != 0U && _errors != errors)
        {
            errors = _errors;
            send(State::CONSOLE, errors);
        }
        const int byte{esp.read()};
        if (lengthRx == 0U)
        {
            lengthRx = static_cast<unsigned char>(static_cast<unsigned char>(byte) >> 4U);
            commandRx = static_cast<Command>(static_cast<unsigned char>(byte) & 0b1111U);
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
        if (target <= LegHandler::maxLimit && target >= LegHandler::minLimit)
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
        ToneHandler::play(static_cast<unsigned int>(bufferRx[1U]) | static_cast<unsigned int>(bufferRx[2U]) << 8U);
    }
}

/**
 * @brief Sends a command without a payload over Serial1.
 *
 * @param state Command to send.
 */
void ConsoleHandler::send(State state) { esp.write(static_cast<unsigned char>(state)); }

/**
 * @brief Writes a command with one payload byte to Serial1.
 *
 * @param state Command identifier.
 * @param byte Payload byte.
 */
void ConsoleHandler::send(State state, unsigned char byte)
{
    esp.write(static_cast<unsigned char>((1U << 4U) | static_cast<unsigned char>(state)));
    esp.write(byte);
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

ConsoleHandler &ConsoleHandler::getInstance()
{
    static ConsoleHandler instance;
    return instance;
}

// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
ConsoleHandler &console{ConsoleHandler::getInstance()};

#undef esp

#endif // __AVR__
