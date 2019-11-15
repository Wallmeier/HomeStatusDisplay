ADC_MODE(ADC_VCC)

#include "HomeStatusDisplay.hpp"

HomeStatusDisplay display("0.9", "HomeStatusDisplay");

// ---------------------------------------------------------------------------------------------------------------------

void setup() { 
    display.begin();
}

// ---------------------------------------------------------------------------------------------------------------------

void loop() {     
    display.work();
}
