# 💡 Bekant

**Bekant** is a hardware and firmware modification for the *IKEA Bekant* desk, combining an ESP32 with the AVR-based *Megadesk* replacement controller.

It provides simple two-button control with height presets, smart-home integration through Home Assistant and MQTT, over-the-air firmware updates, and the ability to flash the Megadesk controller directly from the ESP32.

## Features

- Simple, intuitive two-button controls for manual movement and height presets
- Home Assistant integration via MQTT
- OTA updates for the ESP32
- Remote flashing of the ATtiny841-based Megadesk controller through the ESP32
- RGB visual feedback and audible error indication

## Hardware

- Megadesk replacement controller board
- ESP32 with support for 29-35 V DC input, depending on your desk's power supply
- Logic level shifter for 5V to 3.3V communication

> [!WARNING]
> Do not connect the desk's 29–35 V supply directly to the VIN pin of a typical ESP32 development board. Only use a board specifically designed for this input voltage, or use a suitable buck converter.

### Required hardware

An [Megadesk](https://tinkertown.ca/products/megadesk?variant=43985640554635) replacement controller is required for this mod. The ESP32 is used to control the Megadesk controller and provide smart-home features.

It is recommended to get an ESP32 board with support for up to 35V DC input, depending on the desk's power supply. These aren't very common, but the [Waveshare ESP32-C6-Zero-B](https://www.waveshare.com/esp32-c6-zero-b.htm?sku=34981) is an excellent choice. Alternatively any ESP32 board can be used, but a buck converter is then required to step down the voltage from 29-35 V DC to 5V DC.

For safe communication between the ESP32 and the Megadesk controller, a logic level shifter is required. The Megadesk controller operates at 5V logic levels, while the ESP32 operates at 3.3V logic levels. A logic level shifter ensures that the signals are properly translated between the two devices. These are commonly available and can be found as breakout boards in various forms from different manufacturers. Tested product families include the [TXS0104E](https://www.ti.com/product/TXS0104E) and [TXS0108E](https://www.ti.com/product/TXS0108E).

The logic level shifter can be hidden inside the stock controller enclosure, only exposing the ESP32 partially sticking out of the enclosure on the back side next to the cable. This mounting position can be beneficial as it provides easy access to the ESP32 USB port while also allowing the ESP32's RGB status LED to light up the underside of the desk, providing visual feedback on the desk's status.

### Installation

- Wire the ESP32 and logic level shifter to the Megadesk replacement controller.
- Configure [`secrets.h`](https://github.com/VIPnytt/Bekant/blob/main/include/esp/secrets.h) with pin assignments.
- Build and upload the ESP32 firmware.
- Flash the Megadesk firmware through the ESP32.
- Verify operation physically and optionally through Home Assistant.

## Wiring

| Pin    | Function               | Comment             |
| ------ | ---------------------- | ------------------- |
| `SCK`  | SPI SCLK / UART TX     | Required            |
| `MISO` | SPI MISO / UART RX     | Required            |
| `MOSI` | SPI MOSI               | Required            |
| `RST`  | AVR reset              | Required            |
| `TPUP` | Up button              | Usually not needed  |
| `TPDN` | Down button            | Usually not needed  |
| `OE`   | Level shifter control  | Advanced users only |
| `ADC`  | Supply voltage monitor | Advanced users only |

During normal operation the `SCK` and `MISO` pins are used for serial communication with the Megadesk controller. The  `RST` pin is normally unused, but can be handy to reset the Megadesk controller if it becomes unresponsive. These three pins in combination with the `MOSI` pin are also used to flash the Megadesk controller through the ESP32. No special programming hardware is required, as the ESP32 can act as a programmer for the Megadesk controller.

The `TPUP` and `TPDN` pins can be connected to enable simulation of physical button presses on the Megadesk controller. There's normally no need to connect these pins, as the ESP32 can control the desk height through the Megadesk controller via serial communication. This is mainly useful for debugging and testing, but also allows for some interesting custom use cases.

Connecting `OE` allows the ESP32 to electrically isolate itself from the Megadesk controller by disabling the level shifter. This is mainly useful for development and debugging.

The IKEA Bekant desk has a bad reputation for having a power supply prone to failure. The `ADC` pin can be connected to the 35V DC input voltage via a voltage divider, allowing the ESP32 to monitor the input voltage and report it through Home Assistant. This can be useful for detecting power supply issues before they cause problems.

### Megadesk pinout diagram

```text
      ┌────────────────────┐
TPUP ─┼ TPUP   ┌───────────┼─ MISO
TPDN ─┼ TPDN   │   ┌───────┼─ SCK
      │        │   │    ┌──┼─ RST
      │      MISO SCK  RST │
      │        +5 MOSI GND │
      │        │   │    └──┼─ 0 V DC
      │  x  x  │   └───────┼─ MOSI
      │        └───────────┼─ +5 V DC
      │ RED+ GND LIN x x   │
      └──┼────┼───┼────────┘
         │    │   └────────── LIN
         │    └────────────── 0 V DC
         └─────────────────── +35 V DC
```

### ESP32 pinout diagram

```text
┌──────────────────────┐
│                  VIN ├─ +35 V DC
│                  3V3 ├─ +3.3 V DC
│                  GND ├─ 0 V DC
│                      │
│             UART/SPI ├─ SCK
│             UART/SPI ├─ MISO
│                  SPI ├─ MOSI
│                      │
│       Digital output ├─ RST
│       Digital output ├─ OE
│                      │
│ Digital input/output ├─ TPUP
│ Digital input/output ├─ TPDN
│                      │
│         Analog input ├─ ADC
└──────────────────────┘
```

### Logic level shifter pinout diagram

```text
   0 V DC ────────┬──────── 0 V DC
+3.3 V DC ────┐   │   ┌──── +5 V DC
           ┌──┴───┴───┴──┐
           │ VCC GND VCC │
      SCK ─┤ A1  ──►  B1 ├─ SCK
     MISO ─┤ A2  ◄──  B2 ├─ MISO
     MOSI ─┤ A3  ──►  B3 ├─ MOSI
      RST ─┤ A4  ──►  B4 ├─ RST
     TPUP ─┤ A5  ◄─►  B5 ├─ TPUP
     TPDN ─┤ A6  ◄─►  B6 ├─ TPDN
       OE ─┤ OE          │
           └─────────────┘
```

### Cable pinout diagram

```text
──────┐
  Red ┼─ +35 V DC
White ┼─ 0 V DC
 Blue ┼─ LIN
──────┘
```

## Software

[PlatformIO IDE](https://platformio.org/platformio-ide) is required. It provides [integrations](https://platformio.org/install/integration) for a wide range of IDEs — use whichever editor you are most comfortable with.

Define pin assignments and credentials in [`secrets.h`](https://github.com/VIPnytt/Bekant/blob/main/include/esp/secrets.h).

Make sure to upload the ESP32 firmware before flashing the Megadesk controller, as the ESP32 is used as a programmer for the Megadesk controller.

### Status LED

ESP32 boards with a WS2812 RGB LED can take advantage of the status LED functionality.

- **Red:** An error has occurred, this can mean the desk has reached a limit, is unresponsive, or that a communication error has occurred.
- **Green:** Desk is moving in response to a physical button press
- **Blue:** The desk is moving autonomously to a preset height or in response to a command
- **White:** Idle
- The light will switch off after a short period of inactivity.

### Home Assistant

Home Assistant with MQTT is recommended for the best experience, but the desk also works fully offline. When the ESP32 successfully connects to MQTT, the desk will be auto-discovered in Home Assistant. The following entities are available, although only the most relevant ones are shown in the default Home Assistant dashboard.

- **Controls**
  - **Height:** Move to a specific height of choice
  - **Preset high:** Move to the preset high height
  - **Preset low:** Move to the preset low height
- **Sensors**
  - **Desk:** Sensor for the desk's current height
  - **Preset high:** Sensor for the preset high height
  - **Preset low:** Sensor for the preset low height
- **Configuration**
  - **Preset high:** Set the preset high height
  - **Preset low:** Set the preset low height
  - **Child lock:** Holds the Megadesk controller in reset, preventing the desk from responding to commands or physical button presses. Releasing the lock allows normal operation again.
  - **Reboot:** Reboots the ESP32
  - **Accessory:** Intended for controlling the logic level shifter's OE pin, but can also be used for other peripherals. Only available when the `OE` pin is defined in `secrets.h`.
- **Diagnostic**
  - **Offset:** Displays the current leg offset height.
  - **Calibrate:** The desk will slowly lower to the lowest physically possible position and then reset the legs height counters.
  - **Wi-Fi signal:** Displays the last reported Wi-Fi RSSI signal strength of the ESP32.
  - **Temperature:** Displays the current temperature of the ESP32.
  - **Encoders:** Displays the current average encoder raw value. The attributes contain more detailed information about each individual encoder.
  - **Serial RX:** Displays the last UART message received from the Megadesk controller.
  - **Serial TX:** Displays the last UART message sent to the Megadesk controller.
  - **Voltage:** Displays the current input voltage of the desk's power supply. Only available when the `ADC` pin is defined in `secrets.h`.
  - **Button down:** Simulates a physical button press on the Megadesk controller's down button. Only available when the `TPDN` pin is defined in `secrets.h`.
  - **Button up:** Simulates a physical button press on the Megadesk controller's up button. Only available when the `TPUP` pin is defined in `secrets.h`.
