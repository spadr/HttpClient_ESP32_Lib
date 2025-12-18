#ifndef PIO_UNIT_TESTING
#if defined(ARDUINO) && !defined(ARDUINO_ARCH_NATIVE)

#include <Arduino.h>
#include "app/App.hpp"

void setup() {
    app::init();
    app::runDemo();
}

void loop() {
    app::tick();
}

#endif // ARDUINO && !ARDUINO_ARCH_NATIVE
#endif // PIO_UNIT_TESTING