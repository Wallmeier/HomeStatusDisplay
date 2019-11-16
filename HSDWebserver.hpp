#ifndef HSDWEBSERVER_H
#define HSDWEBSERVER_H

#ifdef ARDUINO_ARCH_ESP32
#include <WebServer.h>
#else
#include <ESP8266WebServer.h>
#include <ESP8266HTTPUpdateServer.h>
#endif

#include "HSDConfig.hpp"
#include "HSDHtmlHelper.hpp"
#include "HSDLeds.hpp"
#include "HSDMqtt.hpp"

class HSDWebserver {  
public:
    HSDWebserver(HSDConfig& config, const HSDLeds& leds, const HSDMqtt& mqtt);

    void begin();
    void handleClient(unsigned long deviceUptime);
#ifdef HSD_SENSOR_ENABLED
    void setSensorData(float& temp, float& hum);
#endif // HSD_SENSOR_ENABLED  

private:
    bool        addColorMappingEntry();
    bool        addDeviceMappingEntry();
    void        checkReboot();
    bool        deleteColorMappingEntry();
    bool        deleteDeviceMappingEntry();
    void        deliverColorMappingPage();
    void        deliverConfigPage();
    void        deliverDeviceMappingPage();
    void        deliverNotFoundPage();
    void        deliverStatusPage();
    inline bool needAdd() { return m_server.hasArg("add"); }
    inline bool needDelete() { return m_server.hasArg("delete"); }
    inline bool needDeleteAll() { return m_server.hasArg("deleteall"); }
    inline bool needSave() { return m_server.hasArg("save"); }
    inline bool needUndo() { return (m_server.hasArg("undo")); }
    bool        updateDeviceMappingConfig();
    bool        updateMainConfig();

    HSDConfig&              m_config;
    unsigned long           m_deviceUptimeMinutes;
    const HSDHtmlHelper     m_html;
#ifdef HSD_SENSOR_ENABLED
    float                   m_lastHum;
    float                   m_lastTemp;
#endif  
    const HSDLeds&          m_leds;
    const HSDMqtt&          m_mqtt;
#ifdef ARDUINO_ARCH_ESP32
    WebServer               m_server;
#else
    ESP8266WebServer        m_server;
    ESP8266HTTPUpdateServer m_updateServer; 
#endif // ARDUINO_ARCH_ESP32    
};

#endif // HSDWEBSERVER_H
