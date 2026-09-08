#pragma once

#ifdef ARDUINO_ARCH_ESP32

#include <NetworkClient.h>
#include <NetworkServer.h>

class IspHandler
{
private:
    static constexpr char stkCrcEop{'\x20'};
    static constexpr char stkFail{'\x11'};
    static constexpr char stkInSync{'\x14'};
    static constexpr char stkNoSync{'\x15'};
    static constexpr char stkOk{'\x10'};

    bool active{false};

    uint16_t eepromSize{0U};
    uint16_t pageSize{0U};

    size_t here{0U};

    std::array<uint8_t, 0b1U << 8U> buffer{0U};

    NetworkClient client{};

    /**
 * Sends a single-byte protocol response.
 * @param byte Response byte to send.
 */

/**
 * Sends an empty protocol response.
 */

/**
 * Reads an EEPROM page of the specified length.
 * @param length Number of bytes to read.
 */

/**
 * Enters device programming mode.
 */

/**
 * Reads a flash page of the specified length.
 * @param length Number of bytes to read.
 */

/**
 * Programs the currently received page.
 */

/**
 * Reads a page according to the current ISP command.
 */

/**
 * Sends the device signature.
 */

/**
 * Processes a universal ISP command.
 */

/**
 * Writes a chunk of data to EEPROM.
 * @param start Starting EEPROM address.
 * @param length Number of bytes to write.
 */

/**
 * Writes data to flash memory.
 * @param length Number of bytes to write.
 */

/**
 * Writes data to EEPROM.
 * @param length Number of bytes to write.
 * @return `true` if the write succeeds, `false` otherwise.
 */

/**
 * Receives a byte from the connected client.
 * @return The received byte.
 */

/**
 * Initializes network-based ISP handling.
 */

/**
 * Processes available network activity and ISP commands.
 */
NetworkServer server{328U};

    void byteReply(uint8_t byte);

    void emptyReply();

    void eepromReadPage(size_t length);

    void enterProgrammingMode();

    void flashReadPage(size_t length);

    void programPage();

    void readPage();

    void readSignature();

    void universal();

    void writeEepromChunk(size_t start, size_t length);

    void writeFlash(size_t length);

    [[nodiscard]] bool writeEeprom(size_t length);

    uint8_t getChar();

public:
    void begin();

    void handle();
};

#endif // ARDUINO_ARCH_ESP32
