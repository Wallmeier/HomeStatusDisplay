#pragma once

#include "HSDConfig.hpp"

class HSDSensor
{
public:
  HSDSensor(const HSDConfig& config);
  void begin();
  bool readSensor(float& temp, float& hum);

private:
  int32_t expectPulse(bool level) const;
  bool read();

  const HSDConfig& m_config;
  uint8_t m_data[5];
  uint32_t m_maxCycles;
  uint8_t m_pin;
};
  
