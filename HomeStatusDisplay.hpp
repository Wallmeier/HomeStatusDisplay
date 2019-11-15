#ifndef HOMESTATUSDISPLAY_H
#define HOMESTATUSDISPLAY_H

#include "HSDConfig.hpp"
#include "HSDWifi.hpp"
#include "HSDWebserver.hpp"
#include "HSDLeds.hpp"
#include "HSDMqtt.hpp"

#ifdef HSD_CLOCK_ENABLED
#include "HSDClock.hpp"
#endif
#ifdef HSD_SENSOR_ENABLED
#include "HSDSensor.hpp"
#endif

class HomeStatusDisplay {
public:
    HomeStatusDisplay(const char* version, const char* identifier);
  
    void begin();
    void work();
  
private:
    unsigned long calcUptime();
    void          checkConnections();
    String        getDevice(const String& statusTopic) const;
    void          handleStatus(const String& device, const String& msg);
#ifdef MQTT_TEST_TOPIC
    void          handleTest(const String& msg);
#endif    
    bool          isStatusTopic(const String& topic) const;
    void          mqttCallback(char* topic, byte* payload, unsigned int length);

#ifdef HSD_CLOCK_ENABLED
    HSDClock*     m_clock;
#endif
    HSDConfig     m_config;
    bool          m_lastMqttConnectionState;
    bool          m_lastWifiConnectionState;
    HSDLeds       m_leds;
    HSDMqtt       m_mqttHandler;
    unsigned long m_oneMinuteTimerLast;
#ifdef HSD_SENSOR_ENABLED
    HSDSensor*    m_sensor;
#endif
    unsigned long m_uptime;
    HSDWebserver  m_webServer;
    HSDWifi       m_wifi;
};

#endif // HOMESTATUSDISPLAY_H
