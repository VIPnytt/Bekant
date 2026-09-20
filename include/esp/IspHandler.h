#pragma once

#ifdef ARDUINO_ARCH_ESP32

#include <NetworkClient.h>
#include <NetworkServer.h>

class IspHandler
{
public:
    /**
     * Starts network-based ISP handling when the reset reason permits it.
     */
    void begin();

    /**
     * Advances the ISP server and programming-session state.
     */
    void handle();

private:
    enum class State : uint8_t // NOLINT(performance-enum-size)
    {
        LISTENING,
        CONNECTED,
        PROGMODE,
        COMPLETE,
    };

    size_t address{0U};
    size_t eepromSize{0U};
    size_t pageSize{0U};

    std::array<uint8_t, 0b1U << 8U> buffer{0U};

    NetworkServer server{328U};

    State state{State::LISTENING};

    static inline NetworkClient client{};

    void chipErase();

    /**
     * Reads an EEPROM page of the specified length.
     * @param length Number of bytes to read.
     */
    void eepromReadPage(size_t length);

    /**
     * Enters device programming mode after validating the request and connection state.
     */
    void enterProgrammingMode();

    /**
     * Reads a flash page of the specified length.
     * @param length Number of bytes to read.
     */
    void flashReadPage(size_t length);

    void getSignOn();

    /**
     * Ends a valid programming session and schedules an ESP32 restart.
     */
    void leaveProgrammingMode();

    /**
     * Processes the next STK500v1 command from the connected client.
     */
    void process();

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

    void setDevice();

    void setDeviceExtended();

    /**
     * Processes a universal ISP command.
     */
    void universal();

    /**
     * Sends an empty protocol response.
     */
    void validateAndAcknowledge();

    /**
     * Sends a single-byte protocol response.
     * @param byte Response byte to send.
     */
    void validateAndAcknowledge(uint8_t byte);

    /**
     * Writes data to EEPROM.
     * @param length Number of bytes to write.
     * @return `true` if the write succeeds, `false` otherwise.
     */
    void writeEeprom(size_t length);

    /**
     * Writes data to flash memory.
     * @param length Number of bytes to write.
     */
    void writeFlash(size_t length);

    /**
     * Waits for and receives a byte from the connected client.
     *
     * Aborts the ESP32 if the client disconnects before a byte arrives.
     *
     * @return The received byte.
     */
    [[nodiscard]] uint8_t readClient();
};

namespace STK500v1
{
constexpr uint8_t STK_OK{0x10U};
constexpr uint8_t STK_FAILED{0x11U};
constexpr uint8_t STK_UNKNOWN{0x12U};
constexpr uint8_t STK_NODEVICE{0x13U};
constexpr uint8_t STK_INSYNC{0x14U};
constexpr uint8_t STK_NOSYNC{0x15U};
constexpr uint8_t ADC_CHANNEL_ERROR{0x16U};
constexpr uint8_t ADC_MEASURE_OK{0x17U};
constexpr uint8_t PWM_CHANNEL_ERROR{0x18U};
constexpr uint8_t PWM_ADJUST_OK{0x19U};
constexpr uint8_t CRC_EOP{0x20U};
constexpr uint8_t STK_GET_SYNC{0x30U};
constexpr uint8_t STK_GET_SIGN_ON{0x31U};
constexpr uint8_t STK_SET_PARAMETER{0x40U};
constexpr uint8_t STK_GET_PARAMETER{0x41U};
constexpr uint8_t STK_SET_DEVICE{0x42U};
constexpr uint8_t STK_SET_DEVICE_EXT{0x45U};
constexpr uint8_t STK_ENTER_PROGMODE{0x50U};
constexpr uint8_t STK_LEAVE_PROGMODE{0x51U};
constexpr uint8_t STK_CHIP_ERASE{0x52U};
constexpr uint8_t STK_CHECK_AUTOINC{0x53U};
constexpr uint8_t STK_LOAD_ADDRESS{0x55U};
constexpr uint8_t STK_UNIVERSAL{0x56U};
constexpr uint8_t STK_UNIVERSAL_MULTI{0x57U};
constexpr uint8_t STK_PROG_FLASH{0x60U};
constexpr uint8_t STK_PROG_DATA{0x61U};
constexpr uint8_t STK_PROG_FUSE{0x62U};
constexpr uint8_t STK_PROG_LOCK{0x63U};
constexpr uint8_t STK_PROG_PAGE{0x64U};
constexpr uint8_t STK_PROG_FUSE_EXT{0x65U};
constexpr uint8_t STK_READ_FLASH{0x70U};
constexpr uint8_t STK_READ_DATA{0x71U};
constexpr uint8_t STK_READ_FUSE{0x72U};
constexpr uint8_t STK_READ_LOCK{0x73U};
constexpr uint8_t STK_READ_PAGE{0x74U};
constexpr uint8_t STK_READ_SIGN{0x75U};
constexpr uint8_t STK_READ_OSCCAL{0x76U};
constexpr uint8_t STK_READ_FUSE_EXT{0x77U};
constexpr uint8_t STK_READ_OSCCAL_EXT{0x78U};
constexpr uint8_t PARAM_HW_VER{0x80U};
constexpr uint8_t PARAM_SW_MAJOR{0x81U};
constexpr uint8_t PARAM_SW_MINOR{0x82U};
constexpr uint8_t PARAM_PROGMODE{0x93U};
} // namespace STK500v1

#endif // ARDUINO_ARCH_ESP32
