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

    /**
 * Network client for the active ISP connection.
 */
NetworkClient client{};

    /**
 * Sends a byte response.
 *
 * @param byte Byte to send.
 */
void byteReply(uint8_t byte);

/**
 * Sends an empty response.
 */
void emptyReply();

/**
 * Reads an EEPROM page of the specified length.
 *
 * @param length Number of bytes to read.
 */
void eepromReadPage(size_t length);

/**
 * Enters programming mode.
 */
void enterProgrammingMode();

/**
 * Reads a flash page of the specified length.
 *
 * @param length Number of bytes to read.
 */
void flashReadPage(size_t length);

/**
 * Programs the current page.
 */
void programPage();

/**
 * Reads a page according to the current programming command.
 */
void readPage();

/**
 * Sends the device signature.
 */
void readSignature();

/**
 * Processes a universal programming command.
 */
void universal();

/**
 * Writes a portion of EEPROM data.
 *
 * @param start Starting EEPROM position.
 * @param length Number of bytes to write.
 */
void writeEepromChunk(size_t start, size_t length);

/**
 * Writes data to flash.
 *
 * @param length Number of bytes to write.
 */
void writeFlash(size_t length);

/**
 * Writes the specified amount of data to EEPROM.
 *
 * @param length Number of bytes to write.
 * @return `true` if the data was written successfully, `false` otherwise.
 */
bool writeEeprom(size_t length);

/**
 * Retrieves the next incoming byte.
 *
 * @return The incoming byte.
 */
uint8_t getChar();

/**
 * Initializes the handler.
 */
void begin();

/**
 * Processes network and programming-protocol activity.
 */
void handle();
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

    bool writeEeprom(size_t length);

    uint8_t getChar();

public:
    void begin();
    void handle();
};

#endif // ARDUINO_ARCH_ESP32
