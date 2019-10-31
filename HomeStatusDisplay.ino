ADC_MODE(ADC_VCC)

#include "HomeStatusDisplay.hpp"

static const char* VERSION = "0.8";
static const char* IDENTIFIER = "HomeStatusDisplay";

HomeStatusDisplay display;

void setup() 
{ 
  display.begin(VERSION, IDENTIFIER);
}

void loop() 
{     
  display.work();
}
