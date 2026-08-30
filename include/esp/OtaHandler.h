#pragma once

#ifdef ARDUINO_ARCH_ESP32

#include <ArduinoOTA.h>

class OtaHandler
{
private:
    ArduinoOTAClass ota;

    static void onStart();

public:
    void begin();
    void handle();
};

#endif // ARDUINO_ARCH_ESP32
