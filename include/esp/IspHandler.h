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

    NetworkServer server{328U};

    /**
     * Sends a single-byte protocol response.
     * @param byte Response byte to send.
     */
    void byteReply(uint8_t byte);

    /**
     * Sends an empty protocol response.
     */
    void emptyReply();

    /**
     * Reads an EEPROM page of the specified length.
     * @param length Number of bytes to read.
     */
    void eepromReadPage(size_t length);

    /**
     * Enters device programming mode.
     */
    void enterProgrammingMode();

    /**
     * Reads a flash page of the specified length.
     * @param length Number of bytes to read.
     */
    void flashReadPage(size_t length);

    /**
     * Programs the currently received page.
     */
    void programPage();

    /**
     * Reads a page according to the current ISP command.
     */
    void readPage();

    /**
     * Sends the device signature.
     */
    void readSignature();

    /**
     * Processes a universal ISP command.
     */
    void universal();

    /**
     * Writes a chunk of data to EEPROM.
     * @param start Starting EEPROM address.
     * @param length Number of bytes to write.
     */
    void writeEepromChunk(size_t start, size_t length);

    /**
     * Writes data to flash memory.
     * @param length Number of bytes to write.
     */
    void writeFlash(size_t length);

    /**
     * Writes data to EEPROM.
     * @param length Number of bytes to write.
     * @return `true` if the write succeeds, `false` otherwise.
     */
    [[nodiscard]] bool writeEeprom(size_t length);

    /**
     * Waits for and receives a byte from the connected client.
     * @return The received byte.
     */
    [[nodiscard]] uint8_t getChar();

public:
    /**
     * Initializes network-based ISP handling.
     */
    void begin();

    /**
     * Processes available network activity and ISP commands.
     */
    void handle();
};

#endif // ARDUINO_ARCH_ESP32
