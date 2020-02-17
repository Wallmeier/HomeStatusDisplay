#include "HSDClock.hpp"

#ifdef ESP32
#include <WiFi.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#endif

HSDClock::HSDClock(const HSDConfig* config) :
    m_config(config),
    m_tm1637(nullptr)
{
}

// ---------------------------------------------------------------------------------------------------------------------

void HSDClock::begin() {
    if (m_config->getClockPinCLK() != m_config->getClockPinDIO()) {
        m_tm1637 = new TM1637(m_config->getClockPinCLK(), m_config->getClockPinDIO());
  
        ezt::setServer(m_config->getClockNTPServer());
        ezt::setInterval(m_config->getClockNTPInterval() * 60);
        ezt::setDebug(INFO, Serial);
        m_local.setPosix(m_config->getClockTimeZone());

        m_tm1637->set(m_config->getClockBrightness());  // set brightness
        m_tm1637->init();
    }
}

// ---------------------------------------------------------------------------------------------------------------------

void HSDClock::handle() {
    static bool initEZ = false;
    static int16_t oldTime = -1;
    
    if (m_tm1637) {    
        if (initEZ || WiFi.isConnected()) {
            initEZ = true;
            // for ezTime
            ezt::events();
        }

        if (WiFi.isConnected()) {
            uint16_t new_time = m_local.hour() * 60 + m_local.minute();
            if (new_time != m_oldTime) {
                m_oldTime = new_time;
                int8_t TimeDisp[4];
                TimeDisp[0] = m_local.hour() / 10;
                TimeDisp[1] = m_local.hour() % 10;
                TimeDisp[2] = m_local.minute() / 10;
                TimeDisp[3] = m_local.minute() % 10;
                m_tm1637->point(1);
                m_tm1637->display(TimeDisp);
            }
        } else {
            if (-1 != m_oldTime) {
                m_oldTime = -1;
                m_tm1637->clearDisplay();
            }
        }
    }
}
