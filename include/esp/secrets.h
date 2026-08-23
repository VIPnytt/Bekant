#pragma once

#ifdef ARDUINO_ARCH_ESP32

// #define PIN_ADC 1 // Optional
// #define PIN_LED 2 // Optional, WaveShare ESP32-C6-Zero-B has a built-in LED on pin 8
#define PIN_MISO 3
#define PIN_MOSI 4
// #define PIN_OE 5 // Optional, for logic level shifter
#define PIN_RST 6
#define PIN_SCK 7
// #define PIN_TPDN 8 // Optional, button down
// #define PIN_TPUP 9 // Optional, button up

#define HOSTNAME "bekant"

#define WIFI_SSID "name"
#define WIFI_KEY "password"

#define MQTT_HOST "mqtt.local"
#define MQTT_USER "username"
#define MQTT_KEY "password"

// #define OTA_KEY "password" // Optional

#endif // ARDUINO_ARCH_ESP32
