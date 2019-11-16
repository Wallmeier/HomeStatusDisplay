#include "HSDClock.hpp"

#ifdef ARDUINO_ARCH_ESP32
#include <WiFi.h>
#else
#include <ESP8266WiFi.h>
#endif // ARDUINO_ARCH_ESP32

HSDClock::HSDClock(const HSDConfig& config) :
    m_config(config),
    m_tm1637(242, 242) // default constructor missing, initialise with unrealistic pin
{
}

// ---------------------------------------------------------------------------------------------------------------------

void HSDClock::begin() {
    m_tm1637 = TM1637(m_config.getClockPinCLK(), m_config.getClockPinDIO());
  
    setServer(m_config.getClockNTPServer());
    setInterval(m_config.getClockNTPInterval() * 60);
    m_local.setPosix(m_config.getClockTimeZone());

    m_tm1637.set(m_config.getClockBrightness());  // set brightness
    m_tm1637.init();
}

// ---------------------------------------------------------------------------------------------------------------------

void HSDClock::handle() {
    // for ezTime
    events();

    if (WiFi.status() == WL_CONNECTED && m_config.getClockPinCLK() != m_config.getClockPinDIO()) {
        uint8_t new_time = m_local.hour() * 100 + m_local.minute();
        if (new_time != m_oldTime) {
            m_oldTime = new_time;
            int8_t TimeDisp[4];
            TimeDisp[0] = m_local.hour() / 10;
            TimeDisp[1] = m_local.hour() % 10;
            TimeDisp[2] = m_local.minute() / 10;
            TimeDisp[3] = m_local.minute() % 10;
            m_tm1637.point(1);
            m_tm1637.display(TimeDisp);
        }
    } else {
        uint8_t new_time = -1;
        if (new_time != m_oldTime) {
            m_oldTime = new_time;
            m_tm1637.clearDisplay();
        }
    }
}
