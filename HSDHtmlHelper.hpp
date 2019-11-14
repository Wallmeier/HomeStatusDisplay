#ifndef HSDHTMLHELPER_H
#define HSDHTMLHELPER_H

#include <Arduino.h>
#include <ESP8266WebServer.h>

#include "HSDConfig.hpp"

class HSDHtmlHelper {
public:
    String getHeader(const char* title, const char* host, const char* version) const;
    String getFooter() const;

    String getColorMappingTableHeader() const;
    String getColorMappingTableEntry(int entryNum, const HSDConfig::ColorMapping* mapping, const String& colorString) const;
    String getColorMappingTableFooter() const;
    String getColorMappingTableAddEntryForm(int newEntryNum, bool isFull) const;

    String getDeviceMappingTableHeader() const;
    String getDeviceMappingTableEntry(int entryNum, const HSDConfig::DeviceMapping* mapping) const;
    String getDeviceMappingTableFooter() const;
    String getDeviceMappingTableAddEntryForm(int newEntryNum, bool isFull) const;

    String getDeleteForm() const;
    String getSaveForm() const;

    String minutes2Uptime(unsigned long minutes) const;
    String ip2String(IPAddress ip) const;
    String behavior2String(HSDConfig::Behavior behavior) const;
    
private:
    String getBehaviorOptions(HSDConfig::Behavior selectedBehavior) const;
    String getColorOptions() const;
};

#endif // HSDHTMLHELPER_H
