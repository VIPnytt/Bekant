#ifdef ARDUINO_ARCH_AVR

#include "avr/LinHandler.h"

#include "avr/constants.h"

/**
 * @brief Initializes the LIN pin and serial interface.
 */
void LinHandler::begin()
{
    pinMode(Pin::lin, OUTPUT);
    Serial.begin(baud);
}

/**
 * @brief Generates a LIN break signal on the configured pin.
 *
 * Stops serial communication while driving the LIN pin low for the break
 * duration, then high for one bit duration before restarting serial
 * communication.
 */
void LinHandler::serialBreak()
{
    Serial.end();
    pinMode(Pin::lin, OUTPUT);
    digitalWrite(Pin::lin, LOW);
    delayMicroseconds(static_cast<unsigned int>(15'000'000UL / baud));
    digitalWrite(Pin::lin, HIGH);
    delayMicroseconds(static_cast<unsigned int>(1'000'000UL / baud));
    Serial.begin(baud);
}

/**
 * @brief Calculates the LIN protected identifier parity bits.
 *
 * @param identifier Six-bit LIN identifier.
 * @return Parity bits positioned in bits 6 and 7.
 */
unsigned char LinHandler::addressParity(unsigned int identifier)
{
    const unsigned int parity0{((identifier >> 0U) & 1U) ^ ((identifier >> 1U) & 1U) ^ ((identifier >> 2U) & 1U) ^
                               ((identifier >> 4U) & 1U)};
    const unsigned int parity1{~(((identifier >> 1U) & 1U) ^ ((identifier >> 3U) & 1U) ^ ((identifier >> 4U) & 1U) ^
                                 ((identifier >> 5U) & 1U)) &
                               1U};
    return static_cast<unsigned char>((parity0 | (parity1 << 1U)) << 6U);
}

/**
 * @brief Reads the next available byte within the specified countdown.
 *
 * @param remainingTime Remaining timeout in microseconds; decremented while waiting.
 * @return int The received byte, or -1 if the countdown expires.
 */
int LinHandler::readWithTimeout(int16_t &remainingTime)
{
    while (Serial.available() == 0)
    {
        delayMicroseconds(100U);
        remainingTime -= 100;
        if (remainingTime <= 0)
        {
            return -1;
        }
    }
    return Serial.read();
}

/**
 * @brief Sends a LIN header for the specified identifier.
 *
 * @param identifier Six-bit LIN identifier used to construct the protected identifier.
 */
void LinHandler::send(unsigned char identifier)
{
    const unsigned char addressByte{
        static_cast<unsigned char>((identifier & 0x3FU) | addressParity(static_cast<unsigned int>(identifier)))};
    serialBreak();
    Serial.write(0x55U);
    Serial.write(addressByte);
    Serial.write(identifier == 0x3CU ? 0xFFU : static_cast<unsigned char>(~addressByte));
    Serial.flush();
    delay(3U);
}

#endif // ARDUINO_ARCH_AVR
