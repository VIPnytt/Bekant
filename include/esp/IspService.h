#pragma once

#ifdef ARDUINO_ARCH_ESP32

#include <NetworkClient.h>

class IspService
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

    void byteReply(uint8_t byte);
    void empty_reply();
    void eeprom_read_page(size_t length);
    void enterProgrammingMode();
    void flash_read_page(size_t length);
    void program_page();
    void read_page();
    void read_signature();
    void universal();
    void write_eeprom_chunk(size_t start, size_t length);
    void write_flash(size_t length);

    bool write_eeprom(size_t length);

    uint8_t getChar();

public:
    void begin();
    void handle();
};

#endif // ARDUINO_ARCH_ESP32
