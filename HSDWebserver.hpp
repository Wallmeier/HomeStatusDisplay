#ifndef HSDWEBSERVER_H
#define HSDWEBSERVER_H

#include <AsyncWebSocket.h>
#include <ESPAsyncWebServer.h>
#ifdef ARDUINO_ARCH_ESP32
#include <SPIFFS.h>
#else
#include <FS.h>
#endif

#include "HSDConfig.hpp"
#include "HSDLeds.hpp"
#include "HSDMqtt.hpp"
#include "QList.h"

class HSDWebserver {  
public:
    enum class StatusClass : uint8_t {
        Device = 0,
        Filesystem, // info ???
        Firmware, // build (Schraubenschl�ssel)
        Flash, // sd_card
        Heap,  // memory
        Mqtt,
        Network, // wifi
        Sensor,
        __Last
    };
    
    struct StatusEntry {
        StatusEntry(StatusClass c, const __FlashStringHelper* l, const String& v, const __FlashStringHelper* u = nullptr, const __FlashStringHelper* id = nullptr) : id(id), label(l), type(c), unit(u), value(v) {}
        
        const __FlashStringHelper* id;
        const __FlashStringHelper* label;
        StatusClass                type;
        const __FlashStringHelper* unit;
        String                     value;
    };

    HSDWebserver(HSDConfig& config, const HSDLeds& leds, const HSDMqtt& mqtt);

    void        begin();
    void        ledChange();
    inline void registerStatusEntry(StatusClass type, const __FlashStringHelper* label, const String& value, const __FlashStringHelper* unit = nullptr, const __FlashStringHelper* id = nullptr) { m_statusEntries.push_back(StatusEntry(type, label, value, unit, id)); }
    void        setUptime(unsigned long& deviceUptime);
    void        updateStatusEntry(const __FlashStringHelper* id, const String& value);

private:
    void   createLedArray(JsonArray& leds) const;
    String createUpdateRequest() const;
    String getConfig() const;
    String getTypeName(StatusClass type) const;
    String getUptimeString(unsigned long& uptime) const;
    void   handleUpload(AsyncWebServerRequest* request, const String& filename, size_t index, uint8_t* data, size_t len, bool final);
    void   handleWebSocket(AsyncWebSocket* server, AsyncWebSocketClient* client, AwsEventType type, void* arg, uint8_t* data, size_t len);
    void   importConfig(const String& filename, const String& content) const;
    String processTemplates(const String& key);
    void   saveColorMapping(const JsonArray& colMapping) const;
    void   saveConfig(const JsonObject& config) const;
    void   saveDeviceMapping(const JsonArray& devMapping) const;
    void   setUpdaterError();

    HSDConfig&         m_config;
    const HSDLeds&     m_leds;
    const HSDMqtt&     m_mqtt;
    AsyncWebServer     m_server;
    QList<StatusEntry> m_statusEntries;
    String             m_updaterError;
    AsyncWebSocket     m_ws;
    String             m_wsBuffer[DEFAULT_MAX_WS_CLIENTS];
};

#endif // HSDWEBSERVER_H
