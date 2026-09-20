#ifdef ARDUINO_ARCH_ESP32

#include "esp/IspHandler.h"

#include "esp/DeskService.h"
#include "esp/secrets.h"

#include <ESPmDNS.h>
#include <SPI.h>

/**
 * @brief Starts the ISP TCP server and registers its mDNS service when startup is allowed.
 *
 * When OTA authentication is configured, an abnormal reset leaves the server closed.
 */
void IspHandler::begin()
{
#ifdef OTA_KEY
    const esp_reset_reason_t reason{esp_reset_reason()};
    if (std::ranges::any_of(DeskService::resetAbnormalities,
                            [&reason](esp_reset_reason_t _reason) { return _reason == reason; }))
    {
        return;
    }
#endif // OTA_KEY
    server.begin();
    MDNS.addService("avrisp", "tcp", 328U);
}

/**
 * @brief Advances the ISP server and programming-session state.
 *
 * When OTA authentication is configured, the server stops accepting new clients after the startup window while an
 * active programming session is allowed to finish. Completing that session, or disconnecting during it, restarts the
 * ESP32.
 */
void IspHandler::handle()
{
#ifdef OTA_KEY
    if (server && millis() > 0b1UL << 22U)
    {
        server.end();
        if (state != State::PROGMODE && client.connected() != 0U)
        {
            client.stop();
            state = State::LISTENING;
        }
    }
#endif // OTA_KEY
    switch (state)
    {
    case State::LISTENING:
#ifdef OTA_KEY
        if (!server)
        {
            return;
        }
#endif // OTA_KEY
        if (server.hasClient())
        {
            state = State::CONNECTED;
            desk.safeMode();
            digitalWrite(PIN_RST, HIGH);
            client = server.accept();
            client.setNoDelay(true);
        }
        break;
    case State::CONNECTED:
        if (client.connected() == 0U)
        {
            client.stop();
            state = State::LISTENING;
        }
        else if (client.available() != 0)
        {
            process();
        }
        break;
    case State::PROGMODE:
        if (client.connected() == 0U)
        {
            SPI.end();
            ESP.restart();
        }
        else if (client.available() != 0)
        {
            process();
        }
        break;
    case State::COMPLETE:
        if (client.connected() != 0U)
        {
            vTaskDelay(0b1U << 3U);
            client.stop();
            vTaskDelay(0b1U << 2U);
        }
        ESP.restart();
        break;
    }
}

/**
 * @brief Processes the next STK500v1 command from the connected client.
 */
void IspHandler::process()
{
    switch (readClient())
    {
    case STK500v1::CRC_EOP:
        client.write(STK500v1::STK_NOSYNC);
        break;
    case STK500v1::STK_GET_SYNC:
        validateAndAcknowledge();
        break;
    case STK500v1::STK_GET_SIGN_ON:
        getSignOn();
        break;
    case STK500v1::STK_GET_PARAMETER:
    {
        switch (readClient())
        {
        case STK500v1::PARAM_HW_VER:
            validateAndAcknowledge(2U);
            break;
        case STK500v1::PARAM_SW_MAJOR:
            validateAndAcknowledge(1U);
            break;
        case STK500v1::PARAM_SW_MINOR:
            validateAndAcknowledge(18U);
            break;
        case STK500v1::PARAM_PROGMODE:
            validateAndAcknowledge(static_cast<uint8_t>('S'));
            break;
        default:
            validateAndAcknowledge(0U);
        }
    }
    break;
    case STK500v1::STK_SET_DEVICE:
        setDevice();
        break;
    case STK500v1::STK_SET_DEVICE_EXT:
        setDeviceExtended();
        break;
    case STK500v1::STK_ENTER_PROGMODE:
        enterProgrammingMode();
        break;
    case STK500v1::STK_LEAVE_PROGMODE:
        leaveProgrammingMode();
        break;
    case STK500v1::STK_CHIP_ERASE:
        chipErase();
        break;
    case STK500v1::STK_LOAD_ADDRESS:
        address = readClient();
        address += (0b1U << 8U) * readClient();
        validateAndAcknowledge();
        break;
    case STK500v1::STK_UNIVERSAL:
        universal();
        break;
    case STK500v1::STK_PROG_FLASH:
        static_cast<void>(readClient());
        static_cast<void>(readClient());
        validateAndAcknowledge();
        break;
    case STK500v1::STK_PROG_DATA:
        static_cast<void>(readClient());
        validateAndAcknowledge();
        break;
    case STK500v1::STK_PROG_PAGE:
        programPage();
        break;
    case STK500v1::STK_READ_PAGE:
        readPage();
        break;
    case STK500v1::STK_READ_SIGN:
        readSignature();
        break;
    default:
        if (readClient() == STK500v1::CRC_EOP)
        {
            constexpr std::array<uint8_t, 2U> response{
                STK500v1::STK_INSYNC,
                STK500v1::STK_UNKNOWN,
            };
            client.write(response.data(), response.size());
        }
        else
        {
            client.write(STK500v1::STK_NOSYNC);
        }
    }
}

void IspHandler::chipErase()
{
    if (readClient() == STK500v1::CRC_EOP)
    {
        client.write(STK500v1::STK_INSYNC);
        SPI.transfer(0xACU);
        SPI.transfer(0x80U);
        SPI.transfer(0U);
        SPI.transfer(0U);
        delay(0b1U << 5U);
        client.write(STK500v1::STK_OK);
    }
    else
    {
        client.write(STK500v1::STK_NOSYNC);
    }
}

/**
 * @brief Reads EEPROM data from the target device and sends it to the client.
 *
 * @param length Number of EEPROM bytes to read.
 */
void IspHandler::eepromReadPage(size_t length)
{
    if (readClient() == STK500v1::CRC_EOP)
    {
        if (length >= buffer.size())
        {
            constexpr std::array<uint8_t, 2U> response{
                STK500v1::STK_INSYNC,
                STK500v1::STK_FAILED,
            };
            client.write(response.data(), response.size());
            return;
        }
        client.write(STK500v1::STK_INSYNC);
        std::array<uint8_t, (0b1U << 8U) + 1U> response{};
        const size_t start{address * 2U};
        for (size_t idx{0U}; idx < length; ++idx)
        {
            const size_t _address{start + idx};
            SPI.transfer(0xA0U);
            SPI.transfer((_address >> 8U) & 0xFFU);
            SPI.transfer(_address & 0xFFU);
            response.at(idx) = SPI.transfer(0xFFU);
        }
        response.at(length) = STK500v1::STK_OK;
        client.write(response.data(), length + 1U);
    }
    else
    {
        client.write(STK500v1::STK_NOSYNC);
    }
}

/**
 * @brief Enters the target device's programming mode.
 *
 * A valid command terminator from a connected client initializes SPI and sends the programming-enable command to the
 * target. Other requests receive a no-sync response.
 */
void IspHandler::enterProgrammingMode()
{
    if (readClient() == STK500v1::CRC_EOP)
    {
        client.write(STK500v1::STK_INSYNC);
        if (state == State::CONNECTED)
        {
            state = State::PROGMODE;
            SPI.begin(PIN_SCK, PIN_MISO, PIN_MOSI, gpio_num_t::GPIO_NUM_NC);
            SPI.setFrequency(225'000UL);
            digitalWrite(PIN_RST, LOW);
            delay(0b1U << 5U);
        }
        SPI.transfer(0xACU);
        SPI.transfer(0x53U);
        SPI.transfer(0U);
        SPI.transfer(0U);
        client.write(STK500v1::STK_OK);
    }
    else
    {
        client.write(STK500v1::STK_NOSYNC);
    }
}

/**
 * @brief Reads flash memory and sends the requested bytes to the client.
 *
 * @param length Number of flash bytes to read.
 */
void IspHandler::flashReadPage(size_t length)
{
    if (readClient() == STK500v1::CRC_EOP)
    {
        client.write(STK500v1::STK_INSYNC);
        std::array<uint8_t, (0b1U << 8U) + 1U> response{};
        for (size_t idx{0U}; idx < length; idx += 2U)
        {
            SPI.transfer(0x20U);
            SPI.transfer((address >> 8U) & 0xFFU);
            SPI.transfer(address & 0xFFU);
            response.at(idx) = SPI.transfer(0U);
            SPI.transfer(0x28U);
            SPI.transfer((address >> 8U) & 0xFFU);
            SPI.transfer(address & 0xFFU);
            response.at(idx + 1U) = SPI.transfer(0U);
            ++address;
        }
        response.at(length) = STK500v1::STK_OK;
        client.write(response.data(), length + 1U);
    }
    else
    {
        client.write(STK500v1::STK_NOSYNC);
    }
}

void IspHandler::getSignOn()
{
    if (readClient() == STK500v1::CRC_EOP)
    {
        constexpr std::array<uint8_t, 9U> response{
            STK500v1::STK_INSYNC,
            static_cast<uint8_t>('A'),
            static_cast<uint8_t>('V'),
            static_cast<uint8_t>('R'),
            0x20U,
            static_cast<uint8_t>('I'),
            static_cast<uint8_t>('S'),
            static_cast<uint8_t>('P'),
            STK500v1::STK_OK,
        };
        client.write(response.data(), response.size());
    }
    else
    {
        client.write(STK500v1::STK_NOSYNC);
    }
}

/**
 * @brief Ends a valid programming session and schedules an ESP32 restart.
 *
 * Requests with an invalid terminator or outside programming mode receive a no-sync response.
 */
void IspHandler::leaveProgrammingMode()
{
    if (readClient() == STK500v1::CRC_EOP)
    {
        if (state == State::PROGMODE)
        {
            client.write(STK500v1::STK_INSYNC);
            SPI.end();
            state = State::COMPLETE;
            client.write(STK500v1::STK_OK);
        }
        else
        {
            constexpr std::array<uint8_t, 2U> response{
                STK500v1::STK_INSYNC,
                STK500v1::STK_OK,
            };
            client.write(response.data(), response.size());
        }
    }
    else
    {
        client.write(STK500v1::STK_NOSYNC);
    }
}

/**
 * @brief Programs one EEPROM or flash request using the current ISP address.
 *
 * Reads the byte count and memory type from the connected client, dispatches
 * EEPROM or flash programming, and sends the protocol response.
 */
void IspHandler::programPage()
{
    const size_t length{((0b1U << 8U) * readClient()) + readClient()};
    const uint8_t memoryType{readClient()};
    if (memoryType == static_cast<uint8_t>('E'))
    {
        writeEeprom(length);
    }
    else if (memoryType == static_cast<uint8_t>('F'))
    {
        writeFlash(length);
    }
    else if (readClient() == STK500v1::CRC_EOP)
    {
        constexpr std::array<uint8_t, 2U> response{
            STK500v1::STK_INSYNC,
            STK500v1::STK_FAILED,
        };
        client.write(response.data(), response.size());
    }
    else
    {
        client.write(STK500v1::STK_NOSYNC);
    }
}

/**
 * @brief Waits for and reads the next byte from the connected client.
 *
 * Aborts the ESP32 if the client disconnects before a byte arrives.
 *
 * @return The byte read from the client.
 */
uint8_t IspHandler::readClient()
{
    while (client.available() == 0)
    {
        if (client.connected() == 0U)
        {
            if (state == State::PROGMODE)
            {
                SPI.end();
            }
            ESP.restart();
        }
        vTaskDelay(1U);
    }
    return static_cast<uint8_t>(client.read());
}

/**
 * @brief Reads a requested EEPROM or flash memory range and sends the result to the client.
 */
void IspHandler::readPage()
{
    const size_t length{((0b1U << 8U) * readClient()) + readClient()};
    const uint8_t memoryType{readClient()};
    if (memoryType == static_cast<uint8_t>('E'))
    {
        eepromReadPage(length);
    }
    else if (memoryType == static_cast<uint8_t>('F'))
    {
        flashReadPage(length);
    }
    else if (readClient() == STK500v1::CRC_EOP)
    {
        constexpr std::array<uint8_t, 2U> response{
            STK500v1::STK_INSYNC,
            STK500v1::STK_FAILED,
        };
        client.write(response.data(), response.size());
    }
    else
    {
        client.write(STK500v1::STK_NOSYNC);
    }
}

/**
 * @brief Reads the target device signature and sends it to the client.
 *
 * Sends a no-sync response when the command terminator is invalid.
 */
void IspHandler::readSignature()
{
    if (readClient() == STK500v1::CRC_EOP)
    {
        client.write(STK500v1::STK_INSYNC);
        std::array<uint8_t, 4U> response{};
        for (uint8_t idx{0U}; idx < 3U; ++idx)
        {
            SPI.transfer(0x30U);
            SPI.transfer(0U);
            SPI.transfer(idx);
            response.at(idx) = SPI.transfer(0U);
        }
        response.at(3U) = STK500v1::STK_OK;
        client.write(response.data(), response.size());
    }
    else
    {
        client.write(STK500v1::STK_NOSYNC);
    }
}

void IspHandler::setDevice()
{
    for (size_t idx{0U}; idx < 20U; ++idx)
    {
        buffer.at(idx) = readClient();
    }
    if (readClient() == STK500v1::CRC_EOP)
    {
        client.write(STK500v1::STK_INSYNC);
        pageSize = static_cast<size_t>((static_cast<unsigned int>(buffer.at(12U)) << 8U) | buffer.at(13U));
        eepromSize = static_cast<size_t>((static_cast<unsigned int>(buffer.at(14U)) << 8U) | buffer.at(15U));
        client.write(STK500v1::STK_OK);
    }
    else
    {
        client.write(STK500v1::STK_NOSYNC);
    }
}

void IspHandler::setDeviceExtended()
{
    for (size_t idx{0U}; idx < 5U; ++idx)
    {
        buffer.at(idx) = readClient();
    }
    validateAndAcknowledge();
}

/**
 * @brief Processes a four-byte universal ISP command.
 *
 * @return The SPI response to the command's fourth byte through the standard byte response.
 */
void IspHandler::universal()
{
    for (size_t idx{0U}; idx < 4U; ++idx)
    {
        buffer.at(idx) = readClient();
    }
    if (readClient() == STK500v1::CRC_EOP)
    {
        client.write(STK500v1::STK_INSYNC);
        for (size_t idx{0U}; idx < 3U; ++idx)
        {
            SPI.transfer(buffer.at(idx));
        }
        const std::array<uint8_t, 2U> response{
            SPI.transfer(buffer.at(3U)),
            STK500v1::STK_OK,
        };
        client.write(response.data(), response.size());
    }
    else
    {
        client.write(STK500v1::STK_NOSYNC);
    }
}

/**
 * @brief Sends a successful empty ISP response when the command terminator is valid.
 */
void IspHandler::validateAndAcknowledge()
{
    if (readClient() == STK500v1::CRC_EOP)
    {
        constexpr std::array<uint8_t, 2U> response{
            STK500v1::STK_INSYNC,
            STK500v1::STK_OK,
        };
        client.write(response.data(), response.size());
    }
    else
    {
        client.write(STK500v1::STK_NOSYNC);
    }
}

/**
 * @brief Sends a synchronized response containing one byte.
 *
 * @param byte Byte to include in the response.
 */
void IspHandler::validateAndAcknowledge(uint8_t byte)
{
    if (readClient() == STK500v1::CRC_EOP)
    {
        const std::array<uint8_t, 3U> response{
            STK500v1::STK_INSYNC,
            byte,
            STK500v1::STK_OK,
        };
        client.write(response.data(), response.size());
    }
    else
    {
        client.write(STK500v1::STK_NOSYNC);
    }
}

/**
 * @brief Writes data from the client to EEPROM.
 *
 * @param length Number of bytes to write.
 * @return `true` if the requested length fits within the configured EEPROM size and is written; `false` otherwise.
 */
void IspHandler::writeEeprom(size_t length)
{
    if (length > eepromSize || length > buffer.size())
    {
        for (size_t idx{0U}; idx < length; ++idx)
        {
            static_cast<void>(readClient());
        }
        if (readClient() == STK500v1::CRC_EOP)
        {
            constexpr std::array<uint8_t, 2U> response{
                STK500v1::STK_INSYNC,
                STK500v1::STK_FAILED,
            };
            client.write(response.data(), response.size());
        }
        else
        {
            client.write(STK500v1::STK_NOSYNC);
        }
        return;
    }
    for (size_t idx{0U}; idx < length; ++idx)
    {
        buffer.at(idx) = readClient();
    }
    if (readClient() == STK500v1::CRC_EOP)
    {
        client.write(STK500v1::STK_INSYNC);
        const size_t start{address * 2U};
        for (size_t idx{0U}; idx < length; ++idx)
        {
            const size_t _address{start + idx};
            SPI.transfer(0xC0U);
            SPI.transfer(_address >> 8U);
            SPI.transfer(_address & 0xFFU);
            SPI.transfer(buffer.at(idx));
            delay(0b1U << 3U);
        }
        client.write(STK500v1::STK_OK);
    }
    else
    {
        client.write(STK500v1::STK_NOSYNC);
    }
}

/**
 * @brief Programs a flash page buffer on the target device.
 *
 * Reads the specified number of bytes from the client, programs them as flash
 * words, and commits each affected flash page. The request must have an even
 * length and a valid terminator.
 *
 * @param length Number of flash data bytes to read and program.
 */
void IspHandler::writeFlash(size_t length)
{
    for (size_t idx{0U}; idx < length; ++idx)
    {
        buffer.at(idx) = readClient();
    }
    if (readClient() == STK500v1::CRC_EOP && (length & 1U) == 0U)
    {
        client.write(STK500v1::STK_INSYNC);
        size_t page{address & ~((pageSize / 2U) - 1U)};
        for (size_t idx{0U}; idx < length; idx += 2U)
        {
            if (page != (address & ~((pageSize / 2U) - 1U)))
            {
                SPI.transfer(0x4CU);
                SPI.transfer((page >> 8U) & 0xFFU);
                SPI.transfer(page & 0xFFU);
                SPI.transfer(0U);
                delay(0b1U << 3U);
                page = address & ~((pageSize / 2U) - 1U);
            }
            SPI.transfer(0x40U);
            SPI.transfer((address >> 8U) & 0xFFU);
            SPI.transfer(address & 0xFFU);
            SPI.transfer(buffer.at(idx));
            SPI.transfer(0x48U);
            SPI.transfer((address >> 8U) & 0xFFU);
            SPI.transfer(address & 0xFFU);
            SPI.transfer(buffer.at(idx + 1U));
            ++address;
        }
        SPI.transfer(0x4CU);
        SPI.transfer((page >> 8U) & 0xFFU);
        SPI.transfer(page & 0xFFU);
        SPI.transfer(0U);
        delay(0b1U << 3U);
        client.write(STK500v1::STK_OK);
    }
    else
    {
        client.write(STK500v1::STK_NOSYNC);
    }
}

#endif // ARDUINO_ARCH_ESP32
