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
- Optional macro buttons and power-supply monitoring

## Hardware

The recommended setup consists of:

- An ATtiny841-based [Megadesk](https://tinkertown.ca/products/megadesk?variant=43985640554635) replacement controller board
- An ESP32 board
- A logic level shifter

Because the Megadesk uses 5 V logic and the ESP32 uses 3.3 V, a logic level shifter is required to bridge them safely. The ESP32 must also be powered directly from the desk’s supply.

### ESP32 power

The desk uses a power supply rated for approximately 29–35 V DC, depending on the specific model.

An ESP32 board rated for this input voltage is the preferred solution, such as the [Waveshare ESP32-C6-Zero-B](https://www.waveshare.com/esp32-c6-zero-b.htm?sku=34981).

Alternatively, a standard ESP32 board can be used with a suitable buck converter, or the *Megadesk Companion*, though neither is recommended for new setups.

> [!WARNING]
> Do not connect the desk’s 29–35 V supply directly to an ESP32 board unless it is specifically rated for that input voltage.

### Megadesk Companion

The *Megadesk Companion* add-on board features a built-in ESP32 and level shifting. Its stock wiring supports normal operation but lacks `MOSI` and `RST`, meaning the ESP32 cannot be used to flash the Megadesk AVR out of the box.

Spare soldering pads can be used to add these connections, but these additional lines require external level shifting.

### Logic level shifting

Tested level-shifter families include the [TXS0104E](https://www.ti.com/product/TXS0104E) and [TXS0108E](https://www.ti.com/product/TXS0108E), both available as breakout boards. Other shifters may also work, provided they support:

- UART communication
- SPI programming
- Open-drain control signals

Note that `RST`, `TPUP`, and `TPDN` are open-drain signals. The optional `ADC` connection requires a resistor divider to safely monitor the desk’s supply voltage.

> [!TIP]
> For a compact assembly, solder a level-shifter breakout board directly to the ESP32. Cut a narrow slot in the back of the controller case to let a small section of the level shifter protrude. Solder the ESP32 to this exposed portion so it sits flat against the tabletop’s underside, keeping the rest of the shifter enclosed. This secures the ESP32 firmly against the desk while leaving the USB port accessible for debugging and the RGB LED visible for status feedback.

## Installation

### 1. Wire the hardware

Connect the ESP32 and logic level shifter to the Megadesk using the wiring diagrams below. For voltage monitoring, wire the `ADC` pin through a suitable resistor divider.

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

`MOSI` and `RST` are highly recommended as they allow the ESP32 to flash and reset the Megadesk AVR.

### 2. Configure the firmware

The project uses [PlatformIO IDE](https://platformio.org/platformio-ide), which provides integrations for a wide range of code editors.

Configure the pin assignments and network credentials in [`secrets.h`](https://github.com/VIPnytt/Bekant/blob/main/include/esp/secrets.h) before proceeding.

### 3. Flashing

Build and upload the ESP32 firmware using PlatformIO, then repeat the process for the ATtiny841 AVR. The ESP32 acts as the programmer for the Megadesk, so it must be running first.

If `MOSI` and `RST` are not connected, the AVR must be flashed separately using a dedicated programmer.

## Connections

### Megadesk pinout

On some revisions, `SCK` and `MISO` are also broken out to `RX` and `TX` through series resistors. Direct `SCK`/`MISO` wiring is preferred, as the resistors are best avoided for SPI and unnecessary for UART, though `RX`/`TX` should work just fine.

```text
                    ┌────────── Button 4
                    │ ┌──────── Button 3
                    │ │ ┌────── 0 V DC
                    │ │ │ ┌──── Button down
                    │ │ │ │ ┌── Button up
         ┌──────────┼─┼─┼─┼─┼─┐
         │         10 9 7 6 5 │
   MISO ─┼──────────┐         │
    SCK ─┼──────┐   │    TPUP ┼─ TPUP
    RST ─┼──┐   │   │    TPDN ┼─ TPDN
         │ RST SCK MISO       │
         │ GND MOSI +5        │
 0 V DC ─┼──┘   │   │         │
   MOSI ─┼──────┘   │  x   x  │
+5 V DC ─┼──────────┘         │
         │ TX RX LIN GND RED+ │
         └───────┼────┼───┼───┘
                 │    │   └───── +35 V DC
                 │    └───────── 0 V DC
                 └────────────── LIN
```

### ESP32 connections

Applicable for ESP32 boards natively rated for the desk’s supply voltage.

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

## Status LED

ESP32 boards equipped with a WS2812 RGB LED can utilize it for visual status indication. The LED automatically fades out after a brief period of inactivity.

| Color | Meaning                                                                    |
| ----- | -------------------------------------------------------------------------- |
| White | Desk is idle                                                               |
| Green | Desk is moving in response to a physical button press                      |
| Blue  | Desk is moving autonomously to a preset height or in response to a command |
| Red   | An error has occurred                                                      |

## Home Assistant

While the desk operates fully offline, pairing it with Home Assistant via MQTT provides the best experience. Once the ESP32 connects to MQTT, Home Assistant will automatically discover the desk.

To avoid interface clutter, only a handful of essential entities are enabled by default.

### Controls

| Name        | Description                           | Requirement |
| ----------- | ------------------------------------- | ----------- |
| Height      | Move to a specific height             |             |
| Lower       | Simulate a physical down-button press | `PIN_TPDN`  |
| Preset high | Move to the high preset               |             |
| Preset low  | Move to the low preset                |             |
| Raise       | Simulate a physical up-button press   | `PIN_TPUP`  |
| Tone        | Play the configured tone              |             |

### Sensors

| Name        | Description               | Requirement  |
| ----------- | ------------------------- | ------------ |
| Button 3    | Programmable macro button | Custom panel |
| Button 4    | Programmable macro button | Custom panel |
| Button down | Button press state        |              |
| Button up   | Button press state        |              |
| Desk        | Current desk height       |              |
| Preset high | Configured high preset    |              |
| Preset low  | Configured low preset     |              |

### Configuration

| Name             | Description                                | Requirement |
| ---------------- | ------------------------------------------ | ----------- |
| Output enable    | Control the logic level shifter’s `OE` pin | `PIN_OE`    |
| Power off        | Put the ESP32 into deep sleep              |             |
| Preset high      | Set the high preset                        |             |
| Preset low       | Set the low preset                         |             |
| Reboot           | Reboot the ESP32                           |             |
| Recalibrate legs | Recalibrate the leg encoder sensors        |             |
| Reset            | Hold the Megadesk controller in reset      | `PIN_RST`   |
| Tone duration    | Set the playback duration in seconds       |             |
| Tone frequency   | Set the audio pitch in kHz                 |             |

### Diagnostics

| Name         | Description                       | Requirement |
| ------------ | --------------------------------- | ----------- |
| Firmware     | ESP32 firmware version            |             |
| Issues       | Alerts and error messages         |             |
| Offset       | Current leg offset                |             |
| Position     | Average encoder position          |             |
| Power supply | Desk power-supply input voltage   | `PIN_ADC`   |
| Serial RX    | Last UART message received        |             |
| Serial TX    | Last UART message sent            |             |
| Temperature  | Internal temperature of the ESP32 |             |
| Wi-Fi signal | ESP32 Wi-Fi RSSI                  |             |

## Troubleshooting

### The desk is unresponsive

If a communication error occurs, the desk may soft-lock and ignore movement commands. Restarting the AVR and ESP32 is not enough to clear this state; the desk itself must be power-cycled. Unplug the desk's power supply for at least 20 seconds, then plug it back in. If the desk still refuses to move, check the *Issues* diagnostic entity in Home Assistant for further insight.

### The desk legs are misaligned

If the desk legs are not moving in sync, the encoder sensors may have lost their reference position. Hold both the *up* and *down* buttons simultaneously for about 10 seconds, or use the *Recalibrate legs* configuration entity in Home Assistant to restore proper alignment. The desk will automatically lower itself to the bottom position to recalibrate the sensors. Ensure the area underneath the desk is clear of chairs and equipment before starting this routine.

### AVRdude fails to flash the Megadesk

When `OTA_KEY` is configured, it also acts as a security mechanism for the Arduino ISP server. The ESP32 will only allow AVR flashing for about an hour after a normal startup. If the ESP32 resets abnormally for any reason, the programming port is locked for security. To unlock it, simply trigger a manual restart using the *Reboot* configuration entity in Home Assistant, or power-cycle the desk.
