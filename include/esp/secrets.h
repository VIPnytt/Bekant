#pragma once

/**
 * Definitions for the Bekant desk project.
 * https://github.com/VIPnytt/Bekant
 */

#ifdef ARDUINO_ARCH_ESP32

/**
 * ESP32 pins, please change to match your board and wiring.
 */
#define PIN_MISO 7 // Megadesk Companion: 7
#define PIN_MOSI 4 // Use any unused GPIO if not connected
#define PIN_RST 5  // Use any unused GPIO if not connected
#define PIN_SCK 6  // Megadesk Companion: 6

/**
 * Optional ESP32 pins for additional features.
 */
// #define PIN_ADC 2
// #define PIN_LED 8 // WaveShare ESP32-C6-Zero-B: 8
// #define PIN_OE 3
// #define PIN_TPDN 20
// #define PIN_TPUP 21

/**
 * Wi-Fi configuration.
 */
#define WIFI_SSID "name"
#define WIFI_KEY "password"

/**
 * MQTT configuration. Leave the defaults if MQTT is not used.
 */
#define MQTT_HOST "mqtt.local"
#define MQTT_USER "username"
#define MQTT_KEY "password"

/**
 * Optional OTA authentication password.
 */
// #define OTA_KEY "password"

/**
 * Device name displayed in Home Assistant.
 */
#define NAME "Bekant"

/**
 * Hostname advertised on the local network.
 */
#define HOSTNAME "bekant"

#endif // ARDUINO_ARCH_ESP32
