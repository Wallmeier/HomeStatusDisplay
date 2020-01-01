#include "HSDWebserver.hpp"

#include <ArduinoJson.h>
#include <AsyncJson.h>
#include <SPIFFSEditor.h>
#include <StreamString.h>
#ifdef ARDUINO_ARCH_ESP32
#include <Update.h>
#else
extern "C" uint32_t _FS_start;
extern "C" uint32_t _FS_end;
#include <Updater.h>
#endif // ARDUINO_ARCH_ESP32

// for placeholders 
using namespace std::placeholders; 

HSDWebserver::HSDWebserver(HSDConfig& config, const HSDLeds& leds, const HSDMqtt& mqtt) :
    m_config(config),
    m_leds(leds),
    m_mqtt(mqtt),
    m_server(80),
    m_ws("/ws")
{
}

// ---------------------------------------------------------------------------------------------------------------------

void HSDWebserver::begin() {
    Serial.println(F("Starting WebServer"));
    
    m_ws.onEvent(std::bind(&HSDWebserver::handleWebSocket, this, _1, _2, _3, _4, _5, _6));
    m_server.addHandler(&m_ws);
#ifdef ESP32
    m_server.addHandler(new SPIFFSEditor(SPIFFS));
#elif defined(ESP8266)
    m_server.addHandler(new SPIFFSEditor());
#endif    
    m_server.on("/", HTTP_GET, [=](AsyncWebServerRequest* request) {
        request->send(SPIFFS, "/index.html", String(), false, std::bind(&HSDWebserver::processTemplates, this, _1));
    });
    m_server.on("/config.json", HTTP_GET, [](AsyncWebServerRequest* request) {
        request->send(SPIFFS, "/config.json", "application/json", true);
    });
    m_server.on("/update", HTTP_POST, [=](AsyncWebServerRequest* request) {
        if (Update.hasError()) {
            request->send(200, F("text/html"), String(F("Update error: ")) + m_updaterError);
        } else {
            request->client()->setNoDelay(true);
            request->send(200, F("text/html"), F("<META http-equiv=\"refresh\" content=\"15;URL=/\">Update Success! Rebooting..."));
            delay(100);
            request->client()->stop();
            ESP.restart();
        }
    }, std::bind(&HSDWebserver::handleUpload, this, _1, _2, _3, _4, _5, _6));
    m_server.on("/ajax/config.json", HTTP_GET, [=](AsyncWebServerRequest* request) {
        Serial.println("GET /ajax/config.json");
        AsyncJsonResponse* response = new AsyncJsonResponse(false);
        JsonObject& root = response->getRoot();
        
        JsonArray& gpioArray = root.createNestedArray("gpios");
#ifdef ARDUINO_ARCH_ESP32
        const uint8_t maxGpio = 16;
        const uint8_t gpios[maxGpio] = {4, 5, 12, 13, 14, 15, 16, 17, 18, 19, 23, 25, 26, 27, 32, 33};
#else    
        const uint8_t maxGpio = 11;
        const uint8_t gpios[maxGpio] = {0, 1, 2, 3, 4, 5, 12, 13, 14, 15, 16};
#endif    
        for (uint8_t idx = 0; idx < maxGpio; idx++)
            gpioArray.add(gpios[idx]);        
        
        JsonArray& json = root.createNestedArray("entries");
        auto cfgEntries = m_config.cfgEntries();
        HSDConfig::Group lastGroup = HSDConfig::Group::__Last;
        
        JsonArray* entries = nullptr;
        for (unsigned int index = 0; index < cfgEntries.size(); index++) {
            auto entry = cfgEntries[index];
            if (entry.type != HSDConfig::DataType::ColorMapping && 
                entry.type != HSDConfig::DataType::DeviceMapping) {
                if (lastGroup != entry.group) {
                    lastGroup = entry.group;
                    JsonObject& obj = json.createNestedObject();
                    obj["name"] = m_config.groupDescription(entry.group);
                    entries = &(obj.createNestedArray("entries"));
                }
                JsonObject& obj = entries->createNestedObject();
                obj["key"] = entry.key;
                obj["label"] = entry.label;
                if (entry.pattern != nullptr)
                    obj["pattern"] = entry.pattern;
                if (entry.patternMsg != nullptr)
                    obj["patternMsg"] = entry.patternMsg;
                if (entry.maxVal > 0)
                    obj["maxVal"] = entry.maxVal;
                obj["type"] = static_cast<int>(entry.type);
                switch (entry.type) {
                    case HSDConfig::DataType::String:
                    case HSDConfig::DataType::Password:
                        obj["value"] = *entry.value.string;
                        break;
                        
                    case HSDConfig::DataType::Bool:
                        obj["value"] = *entry.value.boolean;
                        break;
                        
                    case HSDConfig::DataType::Gpio:
                    case HSDConfig::DataType::Slider:
                        obj["value"] = *entry.value.byte;
                        break;
                        
                    case HSDConfig::DataType::Word:
                        obj["value"] = *entry.value.word;
                        break;
                }
            }
        }
        response->setLength();
        request->send(response);
    });
    m_server.on("/ajax/colormapping.json", HTTP_GET, [=](AsyncWebServerRequest* request) {
        Serial.println("GET /ajax/colormapping.json");
        AsyncJsonResponse* response = new AsyncJsonResponse(true);
        JsonArray& colMapping = response->getRoot();
        const QList<HSDConfig::ColorMapping>& colMap = m_config.getColorMap();
        for (unsigned int index = 0; index < colMap.size(); index++) {
            JsonObject& colorMappingEntry = colMapping.createNestedObject(); 
            const HSDConfig::ColorMapping& mapping = colMap[index];
            colorMappingEntry["id"] = index;
            colorMappingEntry["msg"] = mapping.msg;
            colorMappingEntry["col"] = m_config.hex2string(mapping.color);
            colorMappingEntry["beh"] = static_cast<int>(mapping.behavior);
        }
        response->setLength();
        request->send(response);
    });
    m_server.on("/ajax/devicemapping.json", HTTP_GET, [=](AsyncWebServerRequest* request) {
        Serial.println("GET /ajax/devicemapping.json");
        AsyncJsonResponse* response = new AsyncJsonResponse(true);
        JsonArray& devMapping = response->getRoot();
        const QList<HSDConfig::DeviceMapping>& devMap = m_config.getDeviceMap();
        for (unsigned int index = 0; index < devMap.size(); index++) {
            JsonObject& deviceMappingEntry = devMapping.createNestedObject(); 
            const HSDConfig::DeviceMapping& mapping = devMap[index];
            deviceMappingEntry["id"] = index;
            deviceMappingEntry["device"] = mapping.device;
            deviceMappingEntry["led"] = mapping.ledNumber;
        }
        response->setLength();
        request->send(response);
    });
    m_server.on("/ajax/status.json", HTTP_GET, [=](AsyncWebServerRequest* request) {
        Serial.println("GET /ajax/status.json");
        AsyncJsonResponse* response = new AsyncJsonResponse();
        JsonObject& rootObj = response->getRoot();
        JsonArray& types = rootObj.createNestedArray("table");
        StatusClass type = StatusClass::__Last;
        JsonArray* entries = nullptr;
        for (unsigned int index = 0; index < m_statusEntries.size(); index++) {
            const StatusEntry& entry = m_statusEntries[index];
            if (entry.type != type) {
                type = entry.type;
                JsonObject& typeObj = types.createNestedObject();
                typeObj["name"] = getTypeName(entry.type);
                entries = &(typeObj.createNestedArray("entries"));
            }
            JsonObject& obj = entries->createNestedObject();
            obj["label"] = entry.label;
            obj["value"] = entry.value;
            if (entry.unit)
                obj["unit"] = entry.unit;
            if (entry.id)
                obj["id"] = entry.id;
        }
        JsonArray& leds = rootObj.createNestedArray("leds");
        createLedArray(leds);
        response->setLength();
        request->send(response);
    });
    m_server.serveStatic("/css", SPIFFS, "/css"); // TODO CacheControl bzw. LastModified hinzufuegen
    m_server.serveStatic("/js", SPIFFS, "js"); // TODO CacheControl bzw. LastModified hinzufuegen
    m_server.onNotFound([](AsyncWebServerRequest* request) {
        Serial.printf("NOT_FOUND: %S http://%s%s\n", request->methodToString(), request->host().c_str(), request->url().c_str());
        if (request->contentLength()) {
            Serial.printf("_CONTENT_TYPE: %s\n", request->contentType().c_str());
            Serial.printf("_CONTENT_LENGTH: %u\n", request->contentLength());
        }
        request->send(404);
    });    
    m_server.begin();
    
    unsigned long uptime(0);
    char buffer[64];
    m_statusEntries.push_back(StatusEntry(StatusClass::Firmware, F("Version"), "V" HSD_VERSION));
    m_statusEntries.push_back(StatusEntry(StatusClass::Firmware, F("Build date"), __DATE__ " " __TIME__));
#ifdef ESP32
    m_statusEntries.push_back(StatusEntry(StatusClass::Firmware, F("SDK-Version"), ESP.getSdkVersion()));
    snprintf(buffer, 64, "ESP32 (Rev. %d) - %d MHz", ESP.getChipRevision(), ESP.getCpuFreqMHz());
#elif defined(ESP8266)
    m_statusEntries.push_back(StatusEntry(StatusClass::Firmware, F("SDK-Version"), ESP.getCoreVersion() + " / " + String(ESP.getSdkVersion())));
    snprintf(buffer, 64, "ESP8266 - %d MHz", ESP.getCpuFreqMHz());
#endif
    m_statusEntries.push_back(StatusEntry(StatusClass::Device, F("CPU"), buffer));
    m_statusEntries.push_back(StatusEntry(StatusClass::Device, F("Uptime"), getUptimeString(uptime), nullptr, F("uptime")));
#ifdef ESP8266
    m_statusEntries.push_back(StatusEntry(StatusClass::Device, F("Voltage"), String(ESP.getVcc()), F("mV"), F("voltage")));
    snprintf(buffer, 64, "%08X", ESP.getChipId());
    m_statusEntries.push_back(StatusEntry(StatusClass::Device, F("ESP Chip ID"), buffer));
#elif defined(ESP32)
    m_statusEntries.push_back(StatusEntry(StatusClass::Heap, F("Total size"), String(ESP.getHeapSize()), F("Bytes")));
#endif
    m_statusEntries.push_back(StatusEntry(StatusClass::Heap, F("Free"), String(ESP.getFreeHeap()), F("Bytes"), F("freeHeap")));
#ifdef ESP8266    
    m_statusEntries.push_back(StatusEntry(StatusClass::Heap, F("Max free block size"), String(ESP.getMaxFreeBlockSize()), F("Bytes"), F("maxFreeBlock")));
    m_statusEntries.push_back(StatusEntry(StatusClass::Heap, F("Fragmentation"), String(ESP.getHeapFragmentation()), F("%"), F("fragmentation")));
#elif defined(ESP32)
    m_statusEntries.push_back(StatusEntry(StatusClass::Heap, F("Lowest level of free heap since boot"), String(ESP.getMinFreeHeap()), F("Bytes"), F("minFree")));
    m_statusEntries.push_back(StatusEntry(StatusClass::Heap, F("Largest block of heap that can be allocated at once"), String(ESP.getMaxAllocHeap()), F("Bytes"), F("maxAllocHeap")));
#endif
    m_statusEntries.push_back(StatusEntry(StatusClass::Network, F("SSID"), WiFi.SSID()));
    m_statusEntries.push_back(StatusEntry(StatusClass::Network, F("BSSID"), WiFi.BSSIDstr(), nullptr, F("bssid")));
    m_statusEntries.push_back(StatusEntry(StatusClass::Network, F("Channel"), String(WiFi.channel()), nullptr, F("channel")));
    m_statusEntries.push_back(StatusEntry(StatusClass::Network, F("Rssi"), String(WiFi.RSSI()), F("dBm"), F("rssi")));
    m_statusEntries.push_back(StatusEntry(StatusClass::Network, F("MAC address"), WiFi.macAddress()));
    m_statusEntries.push_back(StatusEntry(StatusClass::Network, F("IP address"), WiFi.localIP().toString(), nullptr, F("ip")));
    m_statusEntries.push_back(StatusEntry(StatusClass::Network, F("Subnet Mask"), WiFi.subnetMask().toString(), nullptr, F("subnetMask")));
    m_statusEntries.push_back(StatusEntry(StatusClass::Network, F("Gateway"), WiFi.gatewayIP().toString(), nullptr, F("gateway")));
    m_statusEntries.push_back(StatusEntry(StatusClass::Mqtt, F("Host"), m_config.getMqttServer()));
    m_statusEntries.push_back(StatusEntry(StatusClass::Mqtt, F("Port"), String(m_config.getMqttPort())));
    m_statusEntries.push_back(StatusEntry(StatusClass::Mqtt, F("Status"), m_mqtt.connected() ? F("Connected") : F("Disconnected"), nullptr, F("mqttStatus")));
#ifdef ESP8266
    snprintf(buffer, 64, "%08X", ESP.getFlashChipId());
    m_statusEntries.push_back(StatusEntry(StatusClass::Flash, F("Chip ID"), buffer));
#endif
    m_statusEntries.push_back(StatusEntry(StatusClass::Flash, F("Sketch thinks Flash RAM is size"), String(ESP.getFlashChipSize() / 1024), F("KB")));
#ifdef ESP8266
    m_statusEntries.push_back(StatusEntry(StatusClass::Flash, F("Actual size based on chip Id"), String(ESP.getFlashChipRealSize() / 1024), F("KB"))); 
#endif
    m_statusEntries.push_back(StatusEntry(StatusClass::Flash, F("Used flash memory"), String(ESP.getSketchSize() / 1024), F("KB"))); 
    m_statusEntries.push_back(StatusEntry(StatusClass::Flash, F("Free flash memory"), String(ESP.getFreeSketchSpace() / 1024), F("KB"))); 
    m_statusEntries.push_back(StatusEntry(StatusClass::Flash, F("Flash frequency"), String(ESP.getFlashChipSpeed() / 1000 / 1000), F("MHz"))); 
    FlashMode_t ideMode = ESP.getFlashChipMode();
    String s = ideMode == FM_QIO ? "QIO" : ideMode == FM_QOUT ? "QOUT" : ideMode == FM_DIO ? "DIO" : ideMode == FM_DOUT ? "DOUT" : "UNKNOWN";
    m_statusEntries.push_back(StatusEntry(StatusClass::Flash, F("Flash write mode"), s));
#ifdef ESP8266
    FSInfo fsInfo;
    SPIFFS.info(fsInfo);
    m_statusEntries.push_back(StatusEntry(StatusClass::Filesystem, F("Total KB"), String(fsInfo.totalBytes / 1024.0), F("KB")));
    m_statusEntries.push_back(StatusEntry(StatusClass::Filesystem, F("Used KB"), String(fsInfo.usedBytes / 1024.0), F("KB")));
    m_statusEntries.push_back(StatusEntry(StatusClass::Filesystem, F("Block size"), String(fsInfo.blockSize)));
    m_statusEntries.push_back(StatusEntry(StatusClass::Filesystem, F("Page size"), String(fsInfo.pageSize)));
    m_statusEntries.push_back(StatusEntry(StatusClass::Filesystem, F("Maximum open files"), String(fsInfo.maxOpenFiles)));
    m_statusEntries.push_back(StatusEntry(StatusClass::Filesystem, F("Maximum path length"), String(fsInfo.maxPathLength)));
#elif defined(ESP32)    
    m_statusEntries.push_back(StatusEntry(StatusClass::Filesystem, F("Total KB"), String(SPIFFS.totalBytes() / 1024.0), F("KB")));
    m_statusEntries.push_back(StatusEntry(StatusClass::Filesystem, F("Used KB"), String(SPIFFS.usedBytes() / 1024.0), F("KB")));
#endif    
}

// ---------------------------------------------------------------------------------------------------------------------

void HSDWebserver::updateStatusEntry(const __FlashStringHelper* id, const String& value) {
    for (size_t idx = 0; idx < m_statusEntries.size(); idx++)
        if (m_statusEntries[idx].id == id)
            m_statusEntries[idx].value = value;
}

// ---------------------------------------------------------------------------------------------------------------------

void HSDWebserver::setUptime(unsigned long& deviceUptime) {
    Serial.printf("setUptime(%lu) - ", deviceUptime);
    updateStatusEntry(F("rssi"), String(WiFi.RSSI()));
    updateStatusEntry(F("uptime"), getUptimeString(deviceUptime));
#ifdef ESP32
    updateStatusEntry(F("freeHeap"), String(ESP.getFreeHeap()));
    updateStatusEntry(F("maxAllocHeap"), String(ESP.getMaxAllocHeap()));
    updateStatusEntry(F("minFree"), String(ESP.getMinFreeHeap()));
#endif        
#ifdef ESP8266
    updateStatusEntry(F("fragmentation"), String(ESP.getHeapFragmentation()));
    updateStatusEntry(F("freeHeap"), String(ESP.getFreeHeap()));
    updateStatusEntry(F("maxFreeBlock"), String(ESP.getMaxFreeBlockSize()));
    updateStatusEntry(F("voltage"), String(ESP.getVcc()));
#endif
    
    if (m_ws.availableForWriteAll()) {
        String res = createUpdateRequest();
        Serial.printf("sending uptime to %u connected websockets: '%s'\n", m_ws.count(), res.c_str());
        m_ws.textAll(res);
    } else {
        Serial.println("no websocket connected");
    }
}

// ---------------------------------------------------------------------------------------------------------------------

void HSDWebserver::createLedArray(JsonArray& leds) const {
    uint8_t idx(0);
    for (int ledNr = 0; ledNr < m_config.getNumberOfLeds(); ledNr++) {
        uint32_t color = m_leds.getColor(ledNr);
        HSDConfig::Behavior behavior = m_leds.getBehavior(ledNr);
        if ((LED_COLOR_NONE != color) && (HSDConfig::Behavior::Off != behavior)) {
            JsonObject& ledObj = leds.createNestedObject();
            ledObj["id"] = idx++;
            ledObj["led"] = ledNr;
            ledObj["device"] = m_config.getDevice(ledNr);
            ledObj["col"] = m_config.hex2string(color);
            ledObj["beh"] = static_cast<int>(behavior);
        }
    }        
}

// ---------------------------------------------------------------------------------------------------------------------

String HSDWebserver::createUpdateRequest() const {
    DynamicJsonBuffer jsonBuffer;
    JsonObject& json = jsonBuffer.createObject();
    json["method"] = "update";
    JsonObject& fields = json.createNestedObject("fields");
    for (size_t idx = 0; idx < m_statusEntries.size(); idx++)
        if (m_statusEntries[idx].id != nullptr)
            fields[m_statusEntries[idx].id] = m_statusEntries[idx].value;
       
    String res;
    json.printTo(res);
    return res;
}

// ---------------------------------------------------------------------------------------------------------------------

String HSDWebserver::getConfig() const {
    String tabs = F("\n              <div class=\"mdl-tabs__tab-bar\">\n");
    String form = F("\n              <form id=\"formConfig\">");
    const QList<HSDConfig::ConfigEntry>& entries = m_config.cfgEntries();
    HSDConfig::Group lastGroup = HSDConfig::Group::__Last;
    for (size_t idx = 0; idx < entries.size(); idx++) {
        const HSDConfig::ConfigEntry& entry = entries[idx];
        if (lastGroup != entry.group) {
            String groupName = m_config.groupDescription(entry.group);
            lastGroup = entry.group;
            tabs += F("                <a href=\"#tab-cfg");
            tabs += groupName;
            tabs += F("\" class=\"mdl-tabs__tab");
            if (idx == 0)
                tabs += F(" is-active");
            tabs += F("\">");
            tabs += groupName;
            tabs += F("</a>\n");
            
            form += F("\n                <div class=\"mdl-tabs__panel");
            if (idx == 0)
                form += F(" is-active");
            form += F("\" id=\"tab-cfg");
            form += groupName;
            form += F("\"></div>");
            // form += F("\">%CONFIG-");
            // groupName.toUpperCase();
            // form += groupName;
            // form += F("%\n                </div>\n");
        }
    }
    tabs += F("              </div>\n");
    form += F("\n              </form>");
    return tabs + form;
}

// ---------------------------------------------------------------------------------------------------------------------

String HSDWebserver::getTypeName(StatusClass type) const {
    switch (type) {
        case StatusClass::Device:     return "Device";
        case StatusClass::Filesystem: return "File system (SPIFFS)";
        case StatusClass::Firmware:   return "Firmware";
        case StatusClass::Flash:      return "Flash chip information";
        case StatusClass::Heap:       return "Heap";
        case StatusClass::Mqtt:       return "MQTT";
        case StatusClass::Network:    return "Network";
        case StatusClass::Sensor:     return "Sensor";
        default:                      return "UNKNOWN";
    }
}

// ---------------------------------------------------------------------------------------------------------------------

String HSDWebserver::getUptimeString(unsigned long& uptime) const {
    char buffer[20];
    snprintf(buffer, 20, "%lu days, %02lu:%02lu", uptime / 1440, (uptime / 60) % 24, uptime % 60);
    return String(buffer);
}

// ---------------------------------------------------------------------------------------------------------------------

void HSDWebserver::handleUpload(AsyncWebServerRequest* request, const String& filename, size_t index, uint8_t* data, 
                                size_t len, bool final) {
    if (index == 0) {
        m_updaterError = String();
        Serial.setDebugOutput(true);
        Serial.printf("Update: %s\r\n", filename.c_str());
            
        if (filename == "filesystem") {
#ifndef ARDUINO_ARCH_ESP32
            size_t fsSize = ((size_t) &_FS_end - (size_t) &_FS_start);
#endif                
            SPIFFS.end();
#ifdef ARDUINO_ARCH_ESP32
            if (!Update.begin(UPDATE_SIZE_UNKNOWN, U_SPIFFS)) //start with max available size
#else
            if (!Update.begin(fsSize, U_FS)) //start with max available size
#endif    
                Update.printError(Serial);
        } else {
            uint32_t maxSketchSpace = (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;
            if (!Update.begin(maxSketchSpace, U_FLASH)) //start with max available size
                setUpdaterError();
        }
    } 
    if (!m_updaterError.length()) {
        if (Update.write(data, len) != len)
            setUpdaterError();
    } 
    if (final && !m_updaterError.length()) {
        if (Update.end(true))  //true to set the size to the current progress
            Serial.printf("Update Success\r\nRebooting...\r\n");
        else
            setUpdaterError();
        Serial.setDebugOutput(false);
    }
    delay(0);
}

// ---------------------------------------------------------------------------------------------------------------------

void HSDWebserver::handleWebSocket(AsyncWebSocket* server, AsyncWebSocketClient* client, AwsEventType type, void* arg, 
                                   uint8_t* data, size_t len) {
    if (type == WS_EVT_CONNECT) {
        Serial.printf("ws[%u] connect from %s:%u\n", client->id(), client->remoteIP().toString().c_str(), client->remotePort());
        client->ping();
        client->text(createUpdateRequest());
    } else if (type == WS_EVT_DISCONNECT) {
        Serial.printf("ws[%u] disconnect\n", client->id());
    } else if (type == WS_EVT_ERROR) {
        Serial.printf("ws[%u] error(%u): %s\n", client->id(), *((uint16_t*)arg), (char*)data);
    } else if (type == WS_EVT_PONG) {
        Serial.printf("ws[%u] pong[%u]: %s\n", client->id(), len, (len) ? (char*)data : "");
    } else if (type == WS_EVT_DATA) {
        AwsFrameInfo* info = (AwsFrameInfo*)arg;
        String msg;
        if (info->final && info->index == 0 && info->len == len) {
            // the whole message is in a single frame and we got all of it's data
            Serial.printf("ws[%u] %s-message[%llu]: ", client->id(), (info->opcode == WS_TEXT) ? "text" : "binary", info->len);
            if (info->opcode == WS_TEXT) {
                for (size_t i = 0; i < info->len; i++)
                    msg += (char) data[i];
            }
            Serial.println(msg);
            m_wsBuffer[client->id()] = msg;
        } else {
            // message is comprised of multiple frames or the frame is split into multiple packets
            if (info->index == 0) {
                if (info->num == 0)
                    Serial.printf("ws[%u] %s-message start\n", client->id(), (info->message_opcode == WS_TEXT) ? "text" : "binary");
                Serial.printf("ws[%u] frame[%u] start[%llu]\n", client->id(), info->num, info->len);
            }

            Serial.printf("ws[%u] frame[%u] %s[%llu - %llu]: ", client->id(), info->num, (info->message_opcode == WS_TEXT) ? "text" : "binary", info->index, info->index + len);

            if (info->opcode == WS_TEXT) {
                for (size_t i = 0; i < len; i++)
                    msg += (char) data[i];
            }
            Serial.println(msg);
            m_wsBuffer[client->id()] += msg;

            if ((info->index + len) == info->len) {
                Serial.printf("ws[%u] frame[%u] end[%llu]\n", client->id(), info->num, info->len);
                if (info->final)
                    Serial.printf("ws[%u] %s-message end\n", client->id(), (info->message_opcode == WS_TEXT) ? "text" : "binary");
            }
        }
        if (info->final && ((info->index + len) == info->len)) {
            DynamicJsonBuffer jsonBuffer(m_wsBuffer[client->id()].length() + 1);
            JsonObject& reqObj = jsonBuffer.parseObject(m_wsBuffer[client->id()]);
            String method = reqObj["method"];
            if (method == "importCfg") {
                importConfig(reqObj["filename"], reqObj["data"]);
            } else if (method == "reboot") {
                Serial.println(F("Rebooting ESP..."));
                ESP.restart();
            } else if (method == "saveCfg") {
                saveConfig(reqObj["data"].as<const JsonObject&>());
            } else if (method == "updateTable") {
                String table = reqObj["table"];
                if (table == "deviceMapping")
                    saveDeviceMapping(reqObj["data"].as<const JsonArray&>());
                else if (table == "colorMapping")
                    saveColorMapping(reqObj["data"].as<const JsonArray&>());
                else
                    Serial.printf("Unknown table to update: %s\n", table.c_str());
            } else {
                Serial.printf("Unknown webSocket method: %s\n", method.c_str());
            }
            m_wsBuffer[client->id()] = ""; // cleanup memory
        }
    }
}

// ---------------------------------------------------------------------------------------------------------------------

void HSDWebserver::importConfig(const String& filename, const String& content) const {
    Serial.printf("Import config: %s\r\n", filename.c_str());
    File file = SPIFFS.open(FILENAME_MAINCONFIG, "w+");
    if (!file) {
        Serial.printf("Failed to open %s\r\n", FILENAME_MAINCONFIG);
    } else {
#ifdef ESP8266
        if (file.write(content.c_str()) != content.length())
#elif defined(ESP32)
        if (file.write(reinterpret_cast<const uint8_t*>(content.c_str()), content.length()) != content.length())
#endif            
            Serial.println("Failed to write file");
        file.close();
        m_config.readConfigFile();
    }
}    

// ---------------------------------------------------------------------------------------------------------------------

void HSDWebserver::ledChange() {
    if (m_ws.availableForWriteAll()) {
        DynamicJsonBuffer jsonBuffer;
        JsonObject& json = jsonBuffer.createObject();
        json["method"] = "updLeds";
        createLedArray(json.createNestedArray("data"));
    
        String res;
        json.printTo(res);
        m_ws.textAll(res);
    }
}

// ---------------------------------------------------------------------------------------------------------------------

String HSDWebserver::processTemplates(const String& key) {
    Serial.printf("processTemplates(%s)\n", key.c_str());
    if (key == "CONFIG") {
        return getConfig();
    } else if (key == "VERSION") {
        return "V" HSD_VERSION;
    } else {
        Serial.print(F("UNKNOWN KEY"));
        return "";
    }
}

// ---------------------------------------------------------------------------------------------------------------------

void HSDWebserver::saveColorMapping(const JsonArray& colMapping) const {
    Serial.print(F("Received colormapping: "));
    colMapping.printTo(Serial);
    Serial.println("");
    QList<HSDConfig::ColorMapping> colMap;
    for (size_t i = 0; i < colMapping.size(); i++) {
        const JsonObject& elem = colMapping.get<JsonVariant>(i).as<JsonObject>();
        colMap.push_back(HSDConfig::ColorMapping(elem["msg"].as<String>(), 
                         m_config.string2hex(elem["col"].as<String>()), 
                         static_cast<HSDConfig::Behavior>(elem["beh"].is<int>() ? elem["beh"].as<int>() : elem["beh"].as<String>().toInt())));
    }
    m_config.setColorMap(colMap);
    m_config.writeConfigFile();
}

// ---------------------------------------------------------------------------------------------------------------------

void HSDWebserver::saveConfig(const JsonObject& config) const {
    Serial.print(F("Received configuration: "));
    config.printTo(Serial);
    Serial.println("");
    
    const QList<HSDConfig::ConfigEntry>& entries = m_config.cfgEntries();
    Serial.printf("Config has %u entries\n", entries.size());
    bool needSave(false);
    String key;
    for (size_t idx = 0; idx < entries.size(); idx++) {
        const HSDConfig::ConfigEntry& entry = entries[idx];
        Serial.printf("Checking config entry %d - group %u\n", idx, static_cast<uint8_t>(entry.group));
        if (entry.type == HSDConfig::DataType::ColorMapping || entry.type == HSDConfig::DataType::DeviceMapping)
            continue;
        key = m_config.groupDescription(entry.group) + "." + entry.key;
        Serial.printf("Checking config entry %d - key: %s\n", idx, key.c_str());
        if (!config.containsKey(key)) {
            Serial.print(F("Missing key in configuration: "));
            Serial.println(key);
        } else {
            switch (entries[idx].type) {
                case HSDConfig::DataType::String:
                case HSDConfig::DataType::Password:
                    if (*entry.value.string != config[key].as<String>()) {
                        needSave = true;
                        *entry.value.string = config[key].as<String>();
                    }
                    break;

                case HSDConfig::DataType::Gpio:
                case HSDConfig::DataType::Slider:
                    if (*entry.value.byte != config[key].as<unsigned char>()) {
                        needSave = true;
                        *entry.value.byte = config[key].as<unsigned char>();
                    }
                    break;

                case HSDConfig::DataType::Word:
                    if (*entry.value.word != config[key].as<unsigned short>()) {
                        needSave = true;
                        *entry.value.word = config[key].as<unsigned short>();
                    }
                    break;

                case HSDConfig::DataType::Bool:
                    if (*entry.value.boolean != config[key].as<bool>()) {
                        needSave = true;
                        *entry.value.boolean = config[key].as<bool>();
                    }
                    break;
            }            
        }
    }
    if (needSave) {
        Serial.println(F("Main config has changed, storing it."));
        m_config.writeConfigFile();
    }
}

// ---------------------------------------------------------------------------------------------------------------------

void HSDWebserver::saveDeviceMapping(const JsonArray& devMapping) const {
    Serial.print(F("Received devicemapping: "));
    devMapping.printTo(Serial);
    Serial.println("");
    QList<HSDConfig::DeviceMapping> devMap;
    for (size_t i = 0; i < devMapping.size(); i++) {
        const JsonObject& elem = devMapping.get<JsonVariant>(i).as<JsonObject>();
        devMap.push_back(HSDConfig::DeviceMapping(elem["device"].as<String>(), 
                         elem["led"].is<int>() ? elem["led"].as<int>() : elem["led"].as<String>().toInt()));
    }
    m_config.setDeviceMap(devMap);
    m_config.writeConfigFile();
}

// ---------------------------------------------------------------------------------------------------------------------

void HSDWebserver::setUpdaterError() {
    Update.printError(Serial);
    StreamString str;
    Update.printError(str);
    m_updaterError = str.c_str();
}
