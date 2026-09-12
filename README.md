# 💡 Bekant

**Bekant** is a hardware and firmware modification for the *IKEA Bekant* desk.

It combines an ESP32 with the AVR-based *Megadesk* replacement controller to add network connectivity and smart-home functionality. The Megadesk remains responsible for the desk’s core operation and works fully independently of the ESP32.

## Features

- Two-button control for manual movement and height presets
- Home Assistant integration through MQTT
- Fully functional offline operation
- OTA updates for the ESP32
- Remote flashing of the Megadesk AVR
- RGB status and error indication
- Optional power-supply voltage monitoring

## Hardware

The recommended setup consists of:

- An ATtiny841-based [Megadesk](https://tinkertown.ca/products/megadesk?variant=43985640554635) replacement controller
- An ESP32 board
- A suitable logic level shifter

Megadesk uses 5 V logic. The ESP32 uses 3.3 V. A logic level shifter is required between them.

The ESP32 also needs to be powered from the desk’s supply.

### ESP32 power

The desk provides approximately 24–35 V DC, depending on the power supply.

An ESP32 board rated for this input voltage is the preferred solution. The [Waveshare ESP32-C6-Zero-B](https://www.waveshare.com/esp32-c6-zero-b.htm?sku=34981) is one suitable option.

A conventional ESP32 board can also be used with a suitable buck converter. The *Megadesk Companion* is another supported option. Both are functional, but neither is recommended for new setups.

> [!WARNING]
> Do not connect the desk’s 24–35 V supply directly to an ESP32 board unless it is specifically rated for that input voltage.

### Megadesk Companion

The *Megadesk Companion* has a built-in ESP32 and level shifting. Its stock wiring supports normal operation, but does not include `MOSI` and `RST`, so the ESP32 cannot be used to flash the Megadesk AVR.

Spare soldering pads connected to the ESP32 can be used to add these connections. The additional connections require external level shifting.

### Logic level shifting

Tested level-shifter families include the [TXS0104E](https://www.ti.com/product/TXS0104E) and [TXS0108E](https://www.ti.com/product/TXS0108E).

Other level shifters may also be suitable. The selected device should support:

- UART communication
- SPI programming
- Open-drain control signals

`RST`, `TPUP`, and `TPDN` are open-drain signals.

The optional `ADC` connection requires a resistor divider to monitor the desk’s supply voltage.

## Installation

### 1. Wire the hardware

Connect the ESP32 and logic level shifter to the Megadesk according to the connection diagrams below.

If power-supply monitoring is wanted, connect `ADC` through a suitable resistor divider.

The minimum connections are:

| Pin    | Function               | Requirement |
| ------ | ---------------------- | ----------- |
| `SCK`  | UART TX / SPI SCLK     | Required    |
| `MISO` | UART RX / SPI MISO     | Required    |
| `MOSI` | SPI MOSI               | Recommended |
| `RST`  | AVR reset              | Recommended |
| `TPUP` | Simulate up button     | Special use |
| `TPDN` | Simulate down button   | Special use |
| `ADC`  | Supply voltage monitor | Optional    |
| `OE`   | Level shifter control  | Optional    |

- `MOSI` and `RST` are recommended because they allow the ESP32 to flash and reset the Megadesk AVR.
- `TPUP` and `TPDN` are special-purpose connections for simulating the physical buttons.
- `ADC` and `OE` provide optional functionality.

### 2. Configure the firmware

Configure the pin assignments and credentials in [`secrets.h`](https://github.com/VIPnytt/Bekant/blob/main/include/esp/secrets.h).

### 3. Upload the ESP32

Build and upload the ESP32 firmware using [PlatformIO IDE](https://platformio.org/platformio-ide).

The ESP32 must be running before flashing the Megadesk AVR because the ESP32 acts as its programmer.

### 4. Flash the Megadesk

Use the ESP32 to flash the Megadesk AVR.

`MOSI` and `RST` are required for AVR programming.

### 5. Verify operation

Once both firmware images have been installed, verify that the desk moves correctly.

> [!TIP]
> The level-shifter breakout board can be soldered directly to the ESP32 to create a compact assembly.
> A small opening can be made in the back of the controller case so the level shifter can sit inside the controller compartment while the ESP32 remains exposed next to the cable.
> This keeps the USB port accessible for debugging and allows the RGB LED to provide visual feedback underneath the desk.

## Connections

The ESP32 GPIO assignments depend on the board and are configured in [`secrets.h`](https://github.com/VIPnytt/Bekant/blob/main/include/esp/secrets.h).

### Megadesk pinout

`MISO` and `SCK` are broken out to `TX` and `RX` on newer revisions through series resistors. `MISO`/`SCK` are preferred for new setups, but `TX`/`RX` are compatible.

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
      │ RED+ GND LIN RX TX │
      └──┼────┼───┼────────┘
         │    │   └────────── LIN
         │    └────────────── 0 V DC
         └─────────────────── +35 V DC
```

### ESP32 connections

Only for ESP32 boards rated for the desk’s supply voltage.

```text
┌────────────────┐
│            VIN ├─ +35 V DC
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

### Logic level shifter

The ESP32 side operates at 3.3 V. The Megadesk side operates at 5 V.

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

### Desk controller cable

```text
──────┐
  Red ┼─ +35 V DC
White ┼─ 0 V DC
 Blue ┼─ LIN
──────┘
```

## Software

Bekant uses PlatformIO to build and upload the firmware.

[PlatformIO IDE](https://platformio.org/platformio-ide) provides integrations for a wide range of editors, so use whichever environment you are most comfortable with.

Configure the pin assignments and credentials in [`secrets.h`](https://github.com/VIPnytt/Bekant/blob/main/include/esp/secrets.h).

The ESP32 firmware must be uploaded before the Megadesk AVR can be flashed.

## Status LED

ESP32 boards with a WS2812 RGB LED can use the integrated status indication.

| Color | Meaning                                                                    |
| ----- | -------------------------------------------------------------------------- |
| White | Desk is idle                                                               |
| Green | Desk is moving in response to a physical button press                      |
| Blue  | Desk is moving autonomously to a preset height or in response to a command |
| Red   | An error has occurred                                                      |

The light fades out after a short period of inactivity.

## Home Assistant

Home Assistant with MQTT is recommended for the best experience, but the desk works fully offline without it.

When the ESP32 successfully connects to MQTT, the desk is automatically discovered in Home Assistant.

### Controls

| Name        | Description               |
| ----------- | ------------------------- |
| Height      | Move to a specific height |
| Preset high | Move to the high preset   |
| Preset low  | Move to the low preset    |

### Sensors

| Name        | Description            |
| ----------- | ---------------------- |
| Desk        | Current desk height    |
| Preset high | Configured high preset |
| Preset low  | Configured low preset  |

### Configuration

| Name          | Description                                | Requirement |
| ------------- | ------------------------------------------ | ----------- |
| Output enable | Control the logic level shifter’s `OE` pin | `PIN_OE`    |
| Preset high   | Set the high preset                        |             |
| Preset low    | Set the low preset                         |             |
| Reboot        | Reboot the ESP32                           |             |
| Reset         | Hold the Megadesk controller in reset      | `PIN_RST`   |

### Diagnostics

| Name         | Description                             | Requirement |
| ------------ | --------------------------------------- | ----------- |
| Button down  | Simulate a physical down-button press   | `PIN_TPDN`  |
| Button up    | Simulate a physical up-button press     | `PIN_TPUP`  |
| Calibrate    | Recalibrate the leg encoder sensors     |             |
| Errors       | Currently detected communication errors |             |
| Firmware     | ESP32 firmware version                  |             |
| Offset       | Current leg offset                      |             |
| Position     | Average encoder position                |             |
| Power supply | Desk power-supply input voltage         | `PIN_ADC`   |
| Serial RX    | Last UART message received              |             |
| Serial TX    | Last UART message sent                  |             |
| Temperature  | Internal temperature of the ESP32       |             |
| Wi-Fi signal | ESP32 Wi-Fi RSSI                        |             |

Only a handful of entities are enabled by default to avoid cluttering the Home Assistant interface.
