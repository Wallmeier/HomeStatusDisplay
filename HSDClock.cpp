#include "HSDClock.hpp"
#include <ESP8266WiFi.h>

// NTP Time Configuration
#define TIME_ZONE                1       // (utc+) TZ in hours
#define NTP_UPDATE_INTERVAL_SEC  20*60   // Update time from NTP server every 20 min

// TM1637 Display Ports
#define CLK D5
#define DIO D7

HSDClock::HSDClock()
:
m_tm1637(CLK,DIO)
{
}

void HSDClock::begin()
{
  configTime(TIME_ZONE*3600, 0, "pool.ntp.org");

  m_tm1637.set(3);  // set brightness 2
  m_tm1637.init();
}

void HSDClock::handle()
{
  if (WiFi.status() == WL_CONNECTED)
  {
    time(&now);
    if (difftime(now, last_ntp_update) > NTP_UPDATE_INTERVAL_SEC)
    {
      configTime(TIME_ZONE*3600, 0, "pool.ntp.org");
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
