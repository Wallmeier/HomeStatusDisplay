#ifndef HSDWEBSERVER_H
#define HSDWEBSERVER_H

#include <ESP8266WebServer.h>
#include <ESP8266HTTPUpdateServer.h>

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
    bool        addDeviceMappingEntry();
    void        checkReboot();
    bool        deleteDeviceMappingEntry();
    void        deliverColorMappingPage();
    void        deliverConfigPage();
    void        deliverDeviceMappingPage();
    void        deliverNotFoundPage();
    void        deliverStatusPage();
    inline bool needAdd() const { return m_server.hasArg("add"); }
    inline bool needDelete() const { return m_server.hasArg("delete"); }
    inline bool needDeleteAll() const { return m_server.hasArg("deleteall"); }
    inline bool needSave() const { return m_server.hasArg("save"); }
    inline bool needUndo() const { return (m_server.hasArg("undo")); }
    bool        updateMainConfig();

    
  bool addColorMappingEntry();
  bool deleteColorMappingEntry();

  
  bool updateDeviceMappingConfig();

    HSDConfig&              m_config;
    unsigned long           m_deviceUptimeMinutes;
    const HSDHtmlHelper     m_html;
#ifdef HSD_SENSOR_ENABLED
    float                   m_lastHum;
    float                   m_lastTemp;
#endif  
    const HSDLeds&          m_leds;
    const HSDMqtt&          m_mqtt;
    ESP8266WebServer        m_server;
    ESP8266HTTPUpdateServer m_updateServer; 
};

#endif // HSDWEBSERVER_H
