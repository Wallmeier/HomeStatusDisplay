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
#if defined HSD_BLUETOOTH_ENABLED && defined ARDUINO_ARCH_ESP32
#include "HSDBluetooth.hpp"
#endif

class HomeStatusDisplay {
public:
    HomeStatusDisplay();
  
    void begin();
    void work();
  
private:
    void   calcUptime();
    void   checkMqttConnections();
    String getDevice(const String& statusTopic) const;
    void   handleStatus(const String& device, const String& msg);
#ifdef MQTT_TEST_TOPIC
    void   handleTest(const String& msg);
#endif    
    bool   isStatusTopic(const String& topic) const;
    void   mqttCallback(char* topic, byte* payload, unsigned int length);

#if defined HSD_BLUETOOTH_ENABLED && defined ARDUINO_ARCH_ESP32
    HSDBluetooth* m_bluetooth;
#endif   
#ifdef HSD_CLOCK_ENABLED
    HSDClock*     m_clock;
#endif
    HSDConfig     m_config;
    HSDLeds       m_leds;
    HSDMqtt       m_mqttHandler;
#ifdef HSD_SENSOR_ENABLED
    HSDSensor*    m_sensor;
#endif
    HSDWebserver  m_webServer;
    HSDWifi       m_wifi;
};

#endif // HOMESTATUSDISPLAY_H
