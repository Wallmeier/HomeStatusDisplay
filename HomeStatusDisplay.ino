ADC_MODE(ADC_VCC)

#include "HomeStatusDisplay.hpp"

HomeStatusDisplay display;

// ---------------------------------------------------------------------------------------------------------------------

void setup() { 
    display.begin("0.9", "HomeStatusDisplay");
}

// ---------------------------------------------------------------------------------------------------------------------

void loop() {     
    display.work();
}
