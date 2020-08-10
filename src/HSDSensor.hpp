#ifndef HSDSENSOR_H
#define HSDSENSOR_H

#include <Adafruit_BMP085_U.h>
#include <Adafruit_TSL2561_U.h>

#include "HSDConfig.hpp"
#include "HSDMqtt.hpp"
#include "HSDWebserver.hpp"

class HSDSensor {
public:
    HSDSensor(const HSDConfig* config);
    
    void begin(HSDWebserver* webServer);
    void handle(HSDWebserver* webServer, const HSDMqtt* mqtt);

private:
    int32_t expectPulse(bool level) const;
    void    i2cscan(bool& hasBmp, bool& hasTsl) const;
    void    printSensorDetails(sensor_t& sensor) const;
    bool    read(uint8_t* data) const;
    bool    readSensor(float& temp, float& hum) const;

    Adafruit_BMP085_Unified*  m_bmp;
    const HSDConfig*          m_config;
    uint32_t                  m_maxCycles;
    uint8_t                   m_pin;
    volatile int              m_pirInterruptCounter;
#ifdef ARDUINO_ARCH_ESP32    
    portMUX_TYPE              m_pirMux;
#endif    
    uint8_t                   m_pirPin;
    volatile uint8_t          m_pirValue;
    Adafruit_TSL2561_Unified* m_tsl;

friend void IRAM_ATTR detectsMotion(void* arg);
};
  
#endif // HSDSENSOR_H
