#ifndef HSDWEBSERVER_H
#define HSDWEBSERVER_H

#ifdef ARDUINO_ARCH_ESP32
#include <SPIFFS.h>
#include <WebServer.h>
#else
#include <ESP8266WebServer.h>
#include <FS.h>
#endif

#include "HSDConfig.hpp"
#include "HSDLeds.hpp"
#include "HSDMqtt.hpp"

class HSDWebserver {  
public:
    HSDWebserver(HSDConfig& config, const HSDLeds& leds, const HSDMqtt& mqtt);

    void begin();
    void handleClient(unsigned long deviceUptime);
#ifdef HSD_SENSOR_ENABLED
    inline void setSensorData(const String& data) { m_sensorData = data; }
#endif // HSD_SENSOR_ENABLED  

private:
    String        behavior2String(HSDConfig::Behavior behavior) const;
    void          deliverColorMappingPage();
    void          deliverConfigPage();
    void          deliverDeviceMappingPage();
    void          deliverNotFoundPage();
    void          deliverStatusPage();
    String        getContentType(String filename);
    bool          handleFileRead(String path);
    void          sendHeader(const char* title, bool useTable = false);
    void          setUpdaterError();
    bool          updateMainConfig();

    HSDConfig&              m_config;
    unsigned long           m_deviceUptimeMinutes;
    File                    m_file;
    const HSDLeds&          m_leds;
    const HSDMqtt&          m_mqtt;
#ifdef HSD_SENSOR_ENABLED
    String                  m_sensorData;
#endif  
#ifdef ARDUINO_ARCH_ESP32
    WebServer               m_server;
#else
    ESP8266WebServer        m_server;
#endif // ARDUINO_ARCH_ESP32
    String                  m_updaterError;
};

#endif // HSDWEBSERVER_H
