#ifdef ARDUINO_ARCH_ESP32

#include "esp/IspHandler.h"

#include "esp/DeviceService.h"
#include "esp/secrets.h"

#include <ESPmDNS.h>
#include <SPI.h>

void IspHandler::begin()
{
    digitalWrite(PIN_RST, HIGH);
    MDNS.addService("avrisp", "tcp", 328U);
}

void IspHandler::handle()
{
    if (active)
    {
        if (client.available())
        {
            switch (getChar())
            {
            case 0x20U:
                client.print(stkNoSync);
                break;
            case 0x30U:
                emptyReply();
                break;
            case 0x31U:
                if (getChar() == stkCrcEop)
                {
                    client.print(stkInSync);
                    client.print(F("AVR ISP"));
                    client.print(stkOk);
                }
                break;
            case 0x41U:
            {
                switch (getChar())
                {
                case 0x80U:
                    byteReply(2U);
                    break;
                case 0x81U:
                    byteReply(1U);
                    break;
                case 0x82U:
                    byteReply(18U);
                    break;
                case 0x93U:
                    byteReply('S');
                    break;
                default:
                    byteReply(0U);
                }
            }
            break;
            case 0x42U:
            {
                for (size_t idx{0U}; idx < 20U; ++idx)
                {
                    buffer.at(idx) = getChar();
                }
                pageSize = static_cast<uint16_t>((static_cast<uint16_t>(buffer.at(12U)) << 8U) | buffer.at(13U));
                eepromSize = static_cast<uint16_t>((static_cast<uint16_t>(buffer.at(14U)) << 8U) | buffer.at(15U));
                emptyReply();
            }
            break;
            case 0x45U:
            {
                for (size_t idx{0U}; idx < 5U; ++idx)
                {
                    buffer.at(idx) = getChar();
                }
                emptyReply();
            }
            break;
            case 0x50U:
                enterProgrammingMode();
                emptyReply();
                break;
            case 0x51U:
                SPI.end();
                emptyReply();
                vTaskDelay(5U);
                client.stop();
                break;
            case 0x55U:
                here = getChar();
                here += 256U * getChar();
                emptyReply();
                break;
            case 0x56U:
                universal();
                break;
            case 0x60U:
                getChar();
                getChar();
                emptyReply();
                break;
            case 0x61U:
                getChar();
                emptyReply();
                break;
            case 0x64U:
                programPage();
                break;
            case 0x74U:
                readPage();
                break;
            case 0x75:
                readSignature();
                break;
            default:
                client.print(getChar() == stkCrcEop ? '\x12' : stkNoSync);
            }
        }
        else if (!client.connected())
        {
            SPI.end();
            client.stop();
            ESP.restart();
        }
    }
    else if (DeviceService::avrServer.hasClient())
    {
        Device.safeMode();
        digitalWrite(PIN_RST, HIGH);
        client = DeviceService::avrServer.accept();
        client.setNoDelay(true);
        active = true;
    }
}

void IspHandler::byteReply(uint8_t byte)
{
    if (getChar() == stkCrcEop)
    {
        const std::array<uint8_t, 3U> response{stkInSync, byte, stkOk};
        client.write(response.data(), response.size());
    }
    else
    {
        client.print(stkNoSync);
    }
}

void IspHandler::emptyReply()
{
    if (getChar() == stkCrcEop)
    {
        client.print(stkInSync);
        client.print(stkOk);
    }
    else
    {
        client.print(stkNoSync);
    }
}

void IspHandler::enterProgrammingMode()
{
    SPI.begin(PIN_SCK, PIN_MISO, PIN_MOSI, GPIO_NUM_NC);
    SPI.setFrequency(300'000UL);
    SPI.setHwCs(false);
    digitalWrite(PIN_RST, LOW);
    vTaskDelay(0b1U << 5U);
    SPI.transfer(0xACU);
    SPI.transfer(0x53U);
    SPI.transfer(0U);
    SPI.transfer(0U);
}

void IspHandler::eepromReadPage(size_t length) // NOLINT(readability-make-member-function-const)
{
    std::vector<uint8_t> data(length + 1U);
    const size_t start{here * 2U};
    for (size_t idx{0U}; idx < length; ++idx)
    {
        const size_t addr{start + idx};
        SPI.transfer(0xA0U);
        SPI.transfer((addr >> 8U) & 0xFFU);
        SPI.transfer(addr & 0xFFU);
        data.at(idx) = SPI.transfer(0xFFU);
    }
    data.at(length) = stkOk;
    client.write(data.data(), data.size());
}

void IspHandler::flashReadPage(size_t length)
{
    for (size_t idx{0U}; idx < length; idx += 2U)
    {
        SPI.transfer(0x20U);
        SPI.transfer((here >> 8U) & 0xFFU);
        SPI.transfer(here & 0xFFU);
        const uint8_t low{SPI.transfer(0U)};
        SPI.transfer(0x28U);
        SPI.transfer((here >> 8U) & 0xFFU);
        SPI.transfer(here & 0xFFU);
        const uint8_t high{SPI.transfer(0U)};
        const std::array<uint8_t, 2U> data{low, high};
        client.write(data.data(), data.size());
        ++here;
    }
    const uint8_t status{static_cast<uint8_t>(stkOk)};
    client.write(&status, sizeof(status));
}

uint8_t IspHandler::getChar()
{
    while (!client.available())
    {
        vTaskDelay(1U);
    }
    return static_cast<uint8_t>(client.read());
}

void IspHandler::programPage()
{
    const size_t length{(256U * getChar()) + getChar()};
    const char memtype{getChar()};
    if (memtype == 'E')
    {
        const bool result{writeEeprom(length)};
        if (getChar() == stkCrcEop)
        {
            client.print(stkInSync);
            client.print(result ? stkOk : stkFail);
        }
        else
        {
            client.print(stkNoSync);
        }
    }
    else if (memtype == 'F')
    {
        writeFlash(length);
    }
    else
    {
        client.print(stkFail);
    }
}

void IspHandler::readPage()
{
    const size_t length{(256U * getChar()) + getChar()};
    const char memtype{getChar()};
    if (getChar() != stkCrcEop)
    {
        client.print(stkNoSync);
        return;
    }
    client.print(stkInSync);
    if (memtype == 'E')
    {
        eepromReadPage(length);
    }
    else if (memtype == 'F')
    {
        flashReadPage(length);
    }
}

void IspHandler::readSignature()
{
    if (getChar() != stkCrcEop)
    {
        client.print(stkNoSync);
        return;
    }
    client.print(stkInSync);
    SPI.transfer(0x30U);
    SPI.transfer(0U);
    SPI.transfer(0U);
    client.print(static_cast<char>(SPI.transfer(0U)));
    SPI.transfer(0x30U);
    SPI.transfer(0U);
    SPI.transfer(0x1U);
    client.print(static_cast<char>(SPI.transfer(0U)));
    SPI.transfer(0x30U);
    SPI.transfer(0U);
    SPI.transfer(0x2U);
    client.print(static_cast<char>(SPI.transfer(0U)));
    client.print(stkOk);
}

void IspHandler::universal()
{
    for (size_t idx{0U}; idx < 4U; ++idx)
    {
        buffer.at(idx) = getChar();
    }
    SPI.transfer(buffer.at(0U));
    SPI.transfer(buffer.at(1U));
    SPI.transfer(buffer.at(2U));
    byteReply(SPI.transfer(buffer.at(3U)));
}

bool IspHandler::writeEeprom(size_t length)
{
    if (length > eepromSize)
    {
        return false;
    }
    size_t start{here * 2U};
    while (length > 32U)
    {
        writeEepromChunk(start, 32U);
        start += 32U;
        length -= 32U;
    }
    writeEepromChunk(start, length);
    return true;
}

void IspHandler::writeEepromChunk(size_t start, size_t length)
{
    for (size_t idx{0U}; idx < length; ++idx)
    {
        buffer.at(idx) = getChar();
    }
    for (size_t idx{0U}; idx < length; ++idx)
    {
        const size_t address{start + idx};
        SPI.transfer(0xC0U);
        SPI.transfer(address >> 8U);
        SPI.transfer(address & 0xFFU);
        SPI.transfer(buffer.at(idx));
        vTaskDelay(45U);
    }
}

void IspHandler::writeFlash(size_t length)
{
    for (size_t idx{0U}; idx < length; ++idx)
    {
        buffer.at(idx) = getChar();
    }
    if (getChar() == stkCrcEop && (length & 1U) == 0U)
    {
        client.print(stkInSync);
        size_t page{here & ~((pageSize / 2U) - 1U)};
        for (size_t idx{0U}; idx < length; idx += 2U)
        {
            vTaskDelay(1U);
            if (page != (here & ~((pageSize / 2U) - 1U)))
            {
                SPI.transfer(0x4CU);
                SPI.transfer((page >> 8U) & 0xFFU);
                SPI.transfer(page & 0xFFU);
                SPI.transfer(0U);
                vTaskDelay(0b1U << 4U);
                page = here & ~((pageSize / 2U) - 1U);
            }
            SPI.transfer(0x40U);
            SPI.transfer((here >> 8U) & 0xFFU);
            SPI.transfer(here & 0xFFU);
            SPI.transfer(buffer.at(idx));
            SPI.transfer(0x48U);
            SPI.transfer((here >> 8U) & 0xFFU);
            SPI.transfer(here & 0xFFU);
            SPI.transfer(buffer.at(idx + 1U));
            ++here;
        }
        SPI.transfer(0x4CU);
        SPI.transfer((page >> 8U) & 0xFFU);
        SPI.transfer(page & 0xFFU);
        SPI.transfer(0U);
        vTaskDelay(0b1U << 4U);
        client.print(stkOk);
    }
    else
    {
        client.print(stkNoSync);
    }
}

#endif // ARDUINO_ARCH_ESP32
