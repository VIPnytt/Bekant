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
 * Writes the specified amount of data to EEPROM.
 *
 * @param length Number of bytes to write.
 * @returns `true` if the write succeeds, `false` otherwise.
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
