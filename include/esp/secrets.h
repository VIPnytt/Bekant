#pragma once

/**
 * Definitions for the Bekant desk project.
 * https://github.com/VIPnytt/Bekant
 */

#ifdef ARDUINO_ARCH_ESP32

/**
 * ESP32 pins, please change to match your board and wiring.
 */
#define PIN_MISO 1
#define PIN_MOSI 2
#define PIN_RST 3
#define PIN_SCK 4

/**
 * Optional ESP32 pins for additional features.
 */
// #define PIN_ADC 5
// #define PIN_LED 6 // WaveShare ESP32-C6-Zero-B has a built-in LED on pin 8
// #define PIN_OE 7
// #define PIN_TPDN 8
// #define PIN_TPUP 9

/**
 * Wi-Fi configuration.
 */
#define WIFI_SSID "name"
#define WIFI_KEY "password"

/**
 * MQTT configuration.
 */
#define MQTT_HOST "mqtt.local"
#define MQTT_USER "username"
#define MQTT_KEY "password"

/**
 * Optional OTA authentication password.
 */
// #define OTA_KEY "password"

/**
 * Hostname advertised on the local network.
 */
#define HOSTNAME "bekant"

#endif // ARDUINO_ARCH_ESP32
