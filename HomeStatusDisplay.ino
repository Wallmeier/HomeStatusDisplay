#ifndef ARDUINO_ARCH_ESP32
ADC_MODE(ADC_VCC)
#endif // ARDUINO_ARCH_ESP32

#include "HomeStatusDisplay.hpp"

HomeStatusDisplay display;

// ---------------------------------------------------------------------------------------------------------------------

void setup() { 
    display.begin();
}

// ---------------------------------------------------------------------------------------------------------------------

void loop() {     
    display.work();
}
