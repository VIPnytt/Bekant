#pragma once

#ifdef ARDUINO_ARCH_ESP32

#include <ArduinoJson.h> // NOLINT(misc-include-cleaner)

class IssueHandler
{
private:
    /**
     * Computes the fingerprint used to compare firmware versions.
     *
     * @param characters Version characters to fingerprint.
     * @return The 8-bit firmware fingerprint.
     */
    [[nodiscard]] static constexpr uint8_t fingerprint(std::string_view characters)
    {
        uint8_t hash{0U}; // NOLINT(misc-const-correctness)
        for (const char character : characters)
        {
            hash ^= static_cast<uint8_t>(character);
            hash = static_cast<uint8_t>((hash << 3U) | (hash >> 5U));
        }
        return hash;
    }

    static inline uint8_t consoleTx{0U};
    static inline uint8_t initialization{0U};
    static inline uint8_t legsRx{0U};
    static inline uint8_t node8{0U};
    static inline uint8_t node9{0U};
    static inline uint8_t resetReason{0U};
    static inline uint8_t version{0U};

    static inline hardwareSerial_error_t consoleRx{hardwareSerial_error_t::UART_NO_ERROR};

    void getComs(JsonArray &list);

    void getLegs(JsonArray &list);

    void getResets(JsonArray &list);

public:
    void clear();

    void getIssues(JsonArray &list);

    bool getNode8();

    bool getNode9();

    /**
     * Records a hardware serial receive error.
     * @param error Hardware serial error to record.
     */
    static void onReceiveError(hardwareSerial_error_t error);

    static void setNode8(uint8_t flags);

    static void setNode9(uint8_t flags);

    static void setInitialization(uint8_t flags);

    static void setResetReason(uint8_t flags);

    static void setVersion(uint8_t hash);

    static void setLegsRx(uint8_t flags);

    static void setConsoleTx(uint8_t flags);
};

#endif // ARDUINO_ARCH_ESP32
