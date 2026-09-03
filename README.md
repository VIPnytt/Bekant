# 💡 Bekant

**Bekant** is a hardware and firmware modification for the *IKEA Bekant* desk. It combines an ESP32 with the AVR-based *Megadesk* replacement controller to add network connectivity and smart-home functionality, while the Megadesk remains responsible for the desk’s core operation and works fully independently of the ESP32.

The project provides local two-button control, height presets, Home Assistant integration through MQTT, OTA updates, remote flashing of the Megadesk’s ATtiny841, and optional monitoring of the desk power supply.

## Features

- Two-button control for manual movement and height presets
- Home Assistant integration through MQTT
- Fully functional offline operation
- OTA updates for the ESP32
- Remote flashing of the Megadesk controller through the ESP32
- RGB status and error indication
- Optional power-supply voltage monitoring

## Hardware

A [Megadesk](https://tinkertown.ca/products/megadesk?variant=43985640554635) replacement controller, an ESP32, and a suitable logic level shifter are required.

The Megadesk controller operates at 5 V logic levels while the ESP32 uses 3.3 V, so level shifting is required for communication between them. The optional `ADC` connection requires a resistor divider to monitor the desk’s supply voltage.

### ESP32 power

The desk provides approximately 29–35 V DC, depending on its power supply. An ESP32 board that supports this input voltage is the preferred solution. The [Waveshare ESP32-C6-Zero-B](https://www.waveshare.com/esp32-c6-zero-b.htm?sku=34981) is one suitable option.

A conventional ESP32 board with a suitable buck converter can also be used, but this is not recommended for new setups.

> [!WARNING]
> Do not connect the desk’s 29–35 V supply directly to an ESP32 board unless it is specifically rated for that input voltage.

### Logic level shifting

The Megadesk controller uses 5 V logic levels while the ESP32 uses 3.3 V. Tested level-shifter families include the [TXS0104E](https://www.ti.com/product/TXS0104E) and [TXS0108E](https://www.ti.com/product/TXS0108E).

Other level shifters may also be suitable. The selected solution should support the required UART communication, SPI programming, and open-drain control signals.

## Installation

Wire the ESP32 and level shifter to the Megadesk controller according to the diagrams below. If power-supply monitoring is used, connect the `ADC` pin through a suitable resistor divider.

Configure the pin assignments and credentials in [`secrets.h`](https://github.com/VIPnytt/Bekant/blob/main/include/esp/secrets.h), then build and upload the ESP32 firmware.

The ESP32 must be running before flashing the Megadesk controller because it acts as the programmer for the ATtiny841. Once both firmware images have been installed, verify desk movement before configuring optional features such as Home Assistant.

> [!TIP]
> The level shifter breakout board can be soldered directly to the ESP32, creating one compact assembly. Cut a small opening in the back of the controller case so the level shifter can be placed inside the desk’s controller compartment, leaving only the ESP32 exposed on the outside next to the cable.
>
> This eliminates the need for a separate enclosure while keeping the ESP32 firmly mounted against the underside of the desk. The RGB status LED can light the underside of the desk for visual feedback, and the USB port remains accessible for debugging.

## Connections

| Pin    | Function               | Required |
| ------ | ---------------------- | -------- |
| `SCK`  | SPI SCLK / UART TX     | Yes      |
| `MISO` | SPI MISO / UART RX     | Yes      |
| `MOSI` | SPI MOSI               | Yes      |
| `RST`  | AVR reset              | Yes      |
| `TPUP` | Up button              | No       |
| `TPDN` | Down button            | No       |
| `OE`   | Level shifter control  | No       |
| `ADC`  | Supply voltage monitor | No       |

During normal operation, `SCK` and `MISO` are used for serial communication with the Megadesk controller. Together with `MOSI` and `RST`, they are also used to flash the Megadesk’s ATtiny841 through the ESP32.

`TPUP` and `TPDN` can simulate physical button presses by pulling the corresponding Megadesk inputs low. These connections are normally unnecessary because the ESP32 can control the desk directly through serial communication, but they can be useful for testing and custom control implementations.

Connecting `OE` allows the ESP32 to disable the level shifter and electrically isolate itself from the Megadesk controller. This is mainly useful for development and debugging.

### Megadesk pinout

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

### ESP32 connections

The exact GPIO assignments depend on the ESP32 board and are configured in [`secrets.h`](https://github.com/VIPnytt/Bekant/blob/main/include/esp/secrets.h).

```text
┌────────────────┐
│            VIN ├─ +35 V DC*
│            3V3 ├─ +3.3 V DC
│            GND ├─ 0 V DC
│                │
│       UART/SPI ├─ SCK
│       UART/SPI ├─ MISO
│            SPI ├─ MOSI
│                │
│    Digital I/O ├─ RST
│    Digital I/O ├─ TPUP
│    Digital I/O ├─ TPDN
│                │
│ Digital output ├─ OE
│                │
│   Analog input ├─ ADC
└────────────────┘
```

\* Only for ESP32 boards rated for the desk supply voltage.

### Logic level shifter connections

The ESP32 side operates at 3.3 V and the Megadesk side at 5 V.

```text
   0 V DC ────────┬──────── 0 V DC
+3.3 V DC ────┐   │   ┌──── +5 V DC
           ┌──┴───┴───┴──┐
           │ VCC GND VCC │
      SCK ─┤ A   ──►   B ├─ SCK
     MISO ─┤ A   ◄──   B ├─ MISO
     MOSI ─┤ A   ──►   B ├─ MOSI
      RST ─┤ A   ◄─►   B ├─ RST
     TPUP ─┤ A   ◄─►   B ├─ TPUP
     TPDN ─┤ A   ◄─►   B ├─ TPDN
       OE ─┤ OE          │
           └─────────────┘
```

`RST`, `TPUP`, and `TPDN` are open-drain signals.

### Desk controller cable

```text
──────┐
  Red ┼─ +35 V DC
White ┼─ 0 V DC
 Blue ┼─ LIN
──────┘
```

## Software

[PlatformIO IDE](https://platformio.org/platformio-ide) is required to build and upload the firmware. It provides [integrations](https://platformio.org/install/integration) for a wide range of editors, so use whichever environment you are most comfortable with.

Define pin assignments and credentials in [`secrets.h`](https://github.com/VIPnytt/Bekant/blob/main/include/esp/secrets.h).

Make sure to upload the ESP32 firmware before flashing the Megadesk controller, as the ESP32 is used as the programmer.

### Status LED

ESP32 boards with a WS2812 RGB LED can use the integrated status indication.

| Colour | Meaning                                                                         |
| ------ | ------------------------------------------------------------------------------- |
| White  | The desk is idle.                                                               |
| Green  | The desk is moving in response to a physical button press.                      |
| Blue   | The desk is moving autonomously to a preset height or in response to a command. |
| Red    | An error has occurred.                                                          |

The light fades off after a short period of inactivity.

### Home Assistant

Home Assistant with MQTT is recommended for the best experience, but the desk also works fully offline. When the ESP32 successfully connects to MQTT, the desk is automatically discovered in Home Assistant.

#### Controls

| Name        | Description               |
| ----------- | ------------------------- |
| Height      | Move to a specific height |
| Preset high | Move to the high preset   |
| Preset low  | Move to the low preset    |

#### Sensors

| Name        | Description            |
| ----------- | ---------------------- |
| Desk        | Current desk height    |
| Preset high | Configured high preset |
| Preset low  | Configured low preset  |

#### Configuration

| Name          | Description                                | Requirement |
| ------------- | ------------------------------------------ | ----------- |
| Output enable | Control the logic level shifter’s `OE` pin | `PIN_OE`    |
| Preset high   | Set the high preset                        |             |
| Preset low    | Set the low preset                         |             |
| Reboot        | Reboot the ESP32                           |             |
| Reset         | Hold the Megadesk controller in reset      |             |

#### Diagnostics

| Name           | Description                              | Requirement |
| -------------- | ---------------------------------------- | ----------- |
| Button down    | Simulate a physical down-button press    | `PIN_TPDN`  |
| Button up      | Simulate a physical up-button press      | `PIN_TPUP`  |
| Calibrate      | Recalibrate the leg encoder sensors      |             |
| Encoders       | Raw average leg-encoder height           |             |
| Firmware AVR   | AVR firmware version                     |             |
| Firmware ESP32 | ESP32 firmware version                   |             |
| Offset         | Current leg offset                       |             |
| Power supply   | Input voltage of the desk’s power supply | `PIN_ADC`   |
| Serial RX      | Last UART message received               |             |
| Serial TX      | Last UART message sent                   |             |
| Temperature    | Internal temperature of the ESP32        |             |
| Wi-Fi signal   | ESP32 Wi-Fi RSSI                         |             |

To avoid cluttering the Home Assistant interface, only a handful of entities are enabled by default.
