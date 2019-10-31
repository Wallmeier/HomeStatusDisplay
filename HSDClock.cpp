#include "HSDClock.hpp"
#include <ESP8266WiFi.h>

HSDClock::HSDClock(const HSDConfig& config)
  :
  m_config(config),
  m_tm1637(242, 242) // default constructor missing, initialise with unrealistic pin
{
}

void HSDClock::begin()
{
  m_tm1637 = TM1637(m_config.getClockPinCLK(), m_config.getClockPinDIO());

  setServer(m_config.getClockNTPServer());
  setInterval(m_config.getClockNTPInterval() * 60);
  local.setPosix(m_config.getClockTimeZone());

  m_tm1637.set(m_config.getClockBrightness());  // set brightness
  m_tm1637.init();
}

void HSDClock::handle()
{
  // for ezTime
  events();

  if (WiFi.status() == WL_CONNECTED && m_config.getClockPinCLK() != m_config.getClockPinDIO())
  {
    new_time = local.hour() * 100 + local.minute();
    if (new_time != old_time)
    {
      old_time = new_time;
      TimeDisp[0] = local.hour() / 10;
      TimeDisp[1] = local.hour() % 10;
      TimeDisp[2] = local.minute() / 10;
      TimeDisp[3] = local.minute() % 10;
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
