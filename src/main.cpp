#ifdef ESP8266
#include <Esp.h>
ADC_MODE(ADC_VCC)
#endif

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
