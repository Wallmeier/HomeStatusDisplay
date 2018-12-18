#include "HSDClock.hpp"
#include <ESP8266WiFi.h>

HSDClock::HSDClock(const HSDConfig& config)
:
m_config(config),
m_tm1637(242,242)  // default constructor missing, initialise with unrealistic pin
{
}

void HSDClock::begin()
{
  m_tm1637 = TM1637(m_config.getClockPinCLK(), m_config.getClockPinDIO());
  
  configTime(m_config.getClockTimeZone()*3600, 0, m_config.getClockNTPServer());

  m_tm1637.set(m_config.getClockBrightness());  // set brightness
  m_tm1637.init();
}

void HSDClock::handle()
{
  if (WiFi.status() == WL_CONNECTED && m_config.getClockPinCLK() != m_config.getClockPinDIO())
  {
    time(&now);
    if (difftime(now, last_ntp_update) > m_config.getClockNTPInterval()*60)
    {
      configTime(m_config.getClockTimeZone()*3600, 0, m_config.getClockNTPServer());
      last_ntp_update = now;
    }
    timeinfo = localtime(&now);
    new_time = timeinfo->tm_hour * 100 + timeinfo->tm_min;
    if (new_time != old_time)
    {
      old_time = new_time;
      TimeDisp[0] = timeinfo->tm_hour / 10;
      TimeDisp[1] = timeinfo->tm_hour % 10;
      TimeDisp[2] = timeinfo->tm_min / 10;
      TimeDisp[3] = timeinfo->tm_min % 10;
      m_tm1637.point(1);
      m_tm1637.display(TimeDisp);
    }
  }
  else
  {
    new_time = -1;
    if (new_time != old_time)
    {
      old_time = new_time;
      m_tm1637.clearDisplay();
    }
  }
}
