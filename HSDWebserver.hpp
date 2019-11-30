#ifndef HSDWEBSERVER_H
#define HSDWEBSERVER_H

#ifdef ARDUINO_ARCH_ESP32
#include <WebServer.h>
#else
#include <ESP8266WebServer.h>
#include <ESP8266HTTPUpdateServer.h>
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
    bool          addColorMappingEntry();
    bool          addDeviceMappingEntry();
    String        behavior2String(HSDConfig::Behavior behavior) const;
    void          checkReboot();
    bool          deleteColorMappingEntry();
    bool          deleteDeviceMappingEntry();
    void          deliverColorMappingPage();
    void          deliverCSS();
    void          deliverConfigPage();
    void          deliverDeviceMappingPage();
    void          deliverNotFoundPage();
    void          deliverStatusPage();
    String        getBehaviorOptions(HSDConfig::Behavior selectedBehavior) const;
    String        getColorMappingTableAddEntryForm(int newEntryNum, bool isFull) const;
    String        getDeleteForm() const;
    String        getDeviceMappingTableAddEntryForm(int newEntryNum, bool isFull) const;
    String        getSaveForm() const;
    inline bool   needAdd() { return m_server.hasArg("add"); }
    inline bool   needDelete() { return m_server.hasArg("delete"); }
    inline bool   needDeleteAll() { return m_server.hasArg("deleteall"); }
    inline bool   needSave() { return m_server.hasArg("save"); }
    inline bool   needUndo() { return (m_server.hasArg("undo")); }
    void          sendColorMappingTableEntry(int entryNum, const HSDConfig::ColorMapping* mapping, const String& colorString);
    void          sendDeviceMappingTableEntry(int entryNum, const HSDConfig::DeviceMapping* mapping);
    void          sendHeader(const char* title);
    bool          updateDeviceMappingConfig();
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
    ESP8266HTTPUpdateServer m_updateServer; 
#endif // ARDUINO_ARCH_ESP32    
};

#endif // HSDWEBSERVER_H
