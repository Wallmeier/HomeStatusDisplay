#ifndef HSDWEBSERVER_H
#define HSDWEBSERVER_H

#ifdef ESP32
#include <SPIFFS.h>
#include <WebServer.h>
#elif defined(ESP8266)
#include <ESP8266WebServer.h>
#include <FS.h>
#define WebServer ESP8266WebServer
#endif
#include <WebSocketsServer.h>
#include <vector>

#include "HSDConfig.hpp"
#include "HSDLeds.hpp"
#include "HSDMqtt.hpp"

using namespace std;

class HSDWebserver {  
public:
    enum class StatusClass : uint8_t {
        Device = 0,
        Filesystem, // info ???
        Firmware, // build (Schraubenschluessel)
        Flash, // sd_card
        Heap,  // memory
        Mqtt,
        Network, // wifi
        Sensor,
        __Last
    };
    
    struct StatusEntry {
        StatusEntry(StatusClass c, const char* l, const String& v, const char* u = "", const char* id = "") : id(id), label(l), type(c), unit(u), value(v) {}
        
        const String id;
        const String label;
        StatusClass  type;
        const String unit;
        String       value;
    };

    HSDWebserver(HSDConfig* config, const HSDLeds* leds, const HSDMqtt* mqtt);

    void        begin();
    void        ledChange();
    bool        log(vector<String> lines);
    inline void handle() { m_server->handleClient(); m_ws->loop(); }
    inline void registerStatusEntry(StatusClass type, const char* label, const String& value, const char* unit = "", const char* id = "") { m_statusEntries.push_back(new StatusEntry(type, label, value, unit, id)); }
    void        setUptime(unsigned long& deviceUptime);
    void        updateStatusEntry(const String& id, const String& value);

private:
    void   createLedArray(JsonArray& leds) const;
    String createUpdateRequest() const;
    void   deliverNotFoundPage();
    String getConfig() const;
    String getTypeName(StatusClass type) const;
    String getUptimeString(unsigned long& uptime) const;
    bool   handleFileRead(String path);
    void   handleWebSocket(uint8_t num, WStype_t type, uint8_t * payload, size_t length);
    void   importConfig(const String& filename, const String& content) const;
    String processTemplates(const String& key) const;
    void   saveColorMapping(const JsonArray& colMapping) const;
    void   saveConfig(const JsonObject& config) const;
    void   saveDeviceMapping(const JsonArray& devMapping) const;
    void   sendAndProcessTemplate(const String& filePath);
    void   setUpdaterError();

    HSDConfig*           m_config;
    const HSDLeds*       m_leds;
    const HSDMqtt*       m_mqtt;
    WebServer*           m_server;
    vector<StatusEntry*> m_statusEntries;
    String               m_updaterError;
    WebSocketsServer*    m_ws;
    String               m_wsBuffer[WEBSOCKETS_SERVER_CLIENT_MAX];
};

#endif // HSDWEBSERVER_H
