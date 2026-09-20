#ifdef ARDUINO_ARCH_ESP32

#include "esp/IspHandler.h"

#include "esp/DeskService.h"
#include "esp/secrets.h"

#include <ESPmDNS.h>
#include <SPI.h>

/**
 * @brief Starts the ISP TCP server and registers its mDNS service.
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
 * @brief Processes pending AVR ISP commands or accepts a new client connection.
 *
 * Handles protocol commands for programming and reading the target device, and
 * restarts the ESP32 when the active client disconnects.
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
        if (client.available() != 0)
        {
            process();
        }
        else if (client.connected() == 0U)
        {
            SPI.end();
            ESP.restart();
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
    switch (getChar())
    {
    case STK500v1::CRC_EOP:
        client.write(STK500v1::STK_NOSYNC);
        break;
    case STK500v1::STK_GET_SYNC:
        emptyReply();
        break;
    case STK500v1::STK_GET_SIGN_ON:
        if (getChar() == STK500v1::CRC_EOP)
        {
            client.write(STK500v1::STK_INSYNC);
            client.print("AVR ISP");
            client.write(STK500v1::STK_OK);
        }
        break;
    case STK500v1::STK_GET_PARAMETER:
    {
        switch (getChar())
        {
        case STK500v1::PARAM_HW_VER:
            byteReply(2U);
            break;
        case STK500v1::PARAM_SW_MAJOR:
            byteReply(1U);
            break;
        case STK500v1::PARAM_SW_MINOR:
            byteReply(18U);
            break;
        case STK500v1::PARAM_PROGMODE:
            byteReply(static_cast<uint8_t>('S'));
            break;
        default:
            byteReply(0U);
        }
    }
    break;
    case STK500v1::STK_SET_DEVICE:
    {
        for (size_t idx{0U}; idx < 20U; ++idx)
        {
            buffer.at(idx) = getChar();
        }
        pageSize = static_cast<size_t>((static_cast<unsigned int>(buffer.at(12U)) << 8U) | buffer.at(13U));
        eepromSize = static_cast<size_t>((static_cast<unsigned int>(buffer.at(14U)) << 8U) | buffer.at(15U));
        emptyReply();
    }
    break;
    case STK500v1::STK_SET_DEVICE_EXT:
    {
        for (size_t idx{0U}; idx < 5U; ++idx)
        {
            buffer.at(idx) = getChar();
        }
        emptyReply();
    }
    break;
    case STK500v1::STK_ENTER_PROGMODE:
        enterProgMode();
        break;
    case STK500v1::STK_LEAVE_PROGMODE:
        leaveProgMode();
        break;
    case STK500v1::STK_LOAD_ADDRESS:
        address = getChar();
        address += (0b1U << 8U) * getChar();
        emptyReply();
        break;
    case STK500v1::STK_UNIVERSAL:
        universal();
        break;
    case STK500v1::STK_PROG_FLASH:
        static_cast<void>(getChar());
        static_cast<void>(getChar());
        emptyReply();
        break;
    case STK500v1::STK_PROG_DATA:
        static_cast<void>(getChar());
        emptyReply();
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
        client.write(getChar() == STK500v1::CRC_EOP ? STK500v1::STK_UNKNOWN : STK500v1::STK_NOSYNC);
    }
}

/**
 * @brief Sends a synchronized response containing one byte.
 *
 * @param byte Byte to include in the response.
 */
void IspHandler::byteReply(uint8_t byte)
{
    if (getChar() == STK500v1::CRC_EOP)
    {
        client.write(STK500v1::STK_INSYNC);
        client.write(byte);
        client.write(STK500v1::STK_OK);
    }
    else
    {
        client.write(STK500v1::STK_NOSYNC);
    }
}

/**
 * @brief Sends a successful empty ISP response when the command terminator is valid.
 */
void IspHandler::emptyReply()
{
    if (getChar() == STK500v1::CRC_EOP)
    {
        client.write(STK500v1::STK_INSYNC);
        client.write(STK500v1::STK_OK);
    }
    else
    {
        client.write(STK500v1::STK_NOSYNC);
    }
}

/**
 * @brief Enters the target device's programming mode.
 *
 * Initializes SPI and sends the programming-enable command to the target.
 */
void IspHandler::enterProgMode()
{
    if (state == State::CONNECTED)
    {
        state = State::PROGMODE;
        SPI.begin(PIN_SCK, PIN_MISO, PIN_MOSI, gpio_num_t::GPIO_NUM_NC);
        SPI.setFrequency(225'000UL);
        digitalWrite(PIN_RST, LOW);
        delay(0b1U << 5U);
        SPI.transfer(0xACU);
        SPI.transfer(0x53U);
        SPI.transfer(0U);
        SPI.transfer(0U);
        emptyReply();
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
void IspHandler::eepromReadPage(size_t length) const
{
    std::vector<uint8_t> data(length + 1U);
    const size_t start{address * 2U};
    for (size_t idx{0U}; idx < length; ++idx)
    {
        const size_t _address{start + idx};
        SPI.transfer(0xA0U);
        SPI.transfer((_address >> 8U) & 0xFFU);
        SPI.transfer(_address & 0xFFU);
        data.at(idx) = SPI.transfer(0xFFU);
    }
    data.at(length) = STK500v1::STK_OK;
    client.write(data.data(), data.size());
}

/**
 * @brief Reads flash memory and sends the requested bytes to the client.
 *
 * @param length Number of flash bytes to read.
 */
void IspHandler::flashReadPage(size_t length)
{
    for (size_t idx{0U}; idx < length; idx += 2U)
    {
        SPI.transfer(0x20U);
        SPI.transfer((address >> 8U) & 0xFFU);
        SPI.transfer(address & 0xFFU);
        client.write(SPI.transfer(0U));
        SPI.transfer(0x28U);
        SPI.transfer((address >> 8U) & 0xFFU);
        SPI.transfer(address & 0xFFU);
        client.write(SPI.transfer(0U));
        ++address;
    }
    client.write(STK500v1::STK_OK);
}

/**
 * @brief Waits for and reads the next byte from the connected client.
 *
 * @return uint8_t The byte read from the client.
 */
uint8_t IspHandler::getChar()
{
    while (client.available() == 0)
    {
        vTaskDelay(1U);
    }
    return static_cast<uint8_t>(client.read());
}

void IspHandler::leaveProgMode()
{
    if (state == State::PROGMODE)
    {
        SPI.end();
        emptyReply();
        state = State::COMPLETE;
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
    const size_t length{((0b1U << 8U) * getChar()) + getChar()};
    const uint8_t memoryType{getChar()};
    if (memoryType == static_cast<uint8_t>('E'))
    {
        const bool result{writeEeprom(length)};
        if (getChar() == STK500v1::CRC_EOP)
        {
            client.write(STK500v1::STK_INSYNC);
            client.write(result ? STK500v1::STK_OK : STK500v1::STK_FAILED);
        }
        else
        {
            client.write(STK500v1::STK_NOSYNC);
        }
    }
    else if (memoryType == static_cast<uint8_t>('F'))
    {
        writeFlash(length);
    }
    else
    {
        client.write(STK500v1::STK_FAILED);
    }
}

/**
 * @brief Reads a requested EEPROM or flash memory range and sends the result to the client.
 */
void IspHandler::readPage()
{
    const size_t length{((0b1U << 8U) * getChar()) + getChar()};
    const uint8_t memoryType{getChar()};
    if (getChar() != STK500v1::CRC_EOP)
    {
        client.write(STK500v1::STK_NOSYNC);
        return;
    }
    client.write(STK500v1::STK_INSYNC);
    if (memoryType == static_cast<uint8_t>('E'))
    {
        eepromReadPage(length);
    }
    else if (memoryType == static_cast<uint8_t>('F'))
    {
        flashReadPage(length);
    }
}

/**
 * @brief Reads the target device signature and sends it to the client.
 *
 * Sends a no-sync response when the command terminator is invalid.
 */
void IspHandler::readSignature()
{
    if (getChar() != STK500v1::CRC_EOP)
    {
        client.write(STK500v1::STK_NOSYNC);
        return;
    }
    client.write(STK500v1::STK_INSYNC);
    for (uint8_t idx{0U}; idx < 3U; ++idx)
    {
        SPI.transfer(0x30U);
        SPI.transfer(0U);
        SPI.transfer(idx);
        client.write(SPI.transfer(0U));
    }
    client.write(STK500v1::STK_OK);
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
        buffer.at(idx) = getChar();
    }
    for (size_t idx{0U}; idx < 3U; ++idx)
    {
        SPI.transfer(buffer.at(idx));
    }
    byteReply(SPI.transfer(buffer.at(3U)));
}

/**
 * @brief Writes data from the client to EEPROM.
 *
 * @param length Number of bytes to write.
 * @return `true` if the requested length fits within the configured EEPROM size and is written; `false` otherwise.
 */
bool IspHandler::writeEeprom(size_t length)
{
    if (length > eepromSize)
    {
        return false;
    }
    const size_t start{address * 2U};
    const size_t remainder{length % 32U};
    const size_t end{start + (length - remainder)};
    for (size_t _address{start}; _address < end; _address += 32U)
    {
        writeEepromChunk(_address, 32U);
    }
    if (remainder != 0U)
    {
        writeEepromChunk(end, remainder);
    }
    return true;
}

/**
 * @brief Writes a chunk of data to EEPROM.
 *
 * @param start EEPROM address at which to begin writing.
 * @param length Number of bytes to read and write.
 */
void IspHandler::writeEepromChunk(size_t start, size_t length)
{
    for (size_t idx{0U}; idx < length; ++idx)
    {
        buffer.at(idx) = getChar();
    }
    for (size_t idx{0U}; idx < length; ++idx)
    {
        const size_t _address{start + idx};
        SPI.transfer(0xC0U);
        SPI.transfer(_address >> 8U);
        SPI.transfer(_address & 0xFFU);
        SPI.transfer(buffer.at(idx));
        delay(0b1U << 3U);
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
        buffer.at(idx) = getChar();
    }
    if (getChar() == STK500v1::CRC_EOP && (length & 1U) == 0U)
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
