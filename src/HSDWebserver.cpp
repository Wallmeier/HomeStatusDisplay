#include "HSDWebserver.hpp"

#include <ArduinoJson.h>
#include <StreamString.h>
#ifdef ESP32
#include <Update.h>
#elif defined(ESP8266)
extern "C" uint32_t _FS_start;
extern "C" uint32_t _FS_end;
#include <Updater.h>
#endif
#include <detail\RequestHandlersImpl.h>

// for placeholders 
using namespace std::placeholders; 

HSDWebserver::HSDWebserver(HSDConfig* config, const HSDLeds* leds, const HSDMqtt* mqtt) :
    m_config(config),
    m_leds(leds),
    m_mqtt(mqtt),
    m_server(new WebServer(80)),
    m_ws(new WebSocketsServer(81))
{
}

// ---------------------------------------------------------------------------------------------------------------------

void HSDWebserver::begin() {
    Serial.println("Starting WebServer");
    
    m_ws->onEvent(std::bind(&HSDWebserver::handleWebSocket, this, _1, _2, _3, _4));
    m_server->on("/", HTTP_GET, [=]() {
         sendAndProcessTemplate("/index.html");
    });
    m_server->on("/update", HTTP_POST, [=]() {
        if (Update.hasError()) {
            m_server->send(200, "text/html", String("Update error: ") + m_updaterError);
        } else {
            m_server->client().setNoDelay(true);
            m_server->send(200, "text/html", "<META http-equiv=\"refresh\" content=\"15;URL=/\">Update Success! Rebooting...");
            delay(100);
            m_server->client().stop();
            ESP.restart();
        }
    }, [=]() {
        HTTPUpload& upload = m_server->upload();
        if (upload.status == UPLOAD_FILE_START) {
            m_updaterError = String();
            Serial.setDebugOutput(true);
            Serial.printf("Update: %s\n", upload.filename.c_str());
            
            if (upload.name == "filesystem") {
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
        } else if (upload.status == UPLOAD_FILE_WRITE && !m_updaterError.length()) {
            if (Update.write(upload.buf, upload.currentSize) != upload.currentSize)
                setUpdaterError();
        } else if (upload.status == UPLOAD_FILE_END && !m_updaterError.length()) {
            if (Update.end(true))  //true to set the size to the current progress
                Serial.printf("Update Success: %u\nRebooting...\n", upload.totalSize);
            else
                setUpdaterError();
            Serial.setDebugOutput(false);
        } else if (upload.status == UPLOAD_FILE_ABORTED){
            Update.end();
            Serial.println("Update was aborted");
        } else {
            Serial.printf("Update Failed Unexpectedly (likely broken connection): status=%d\n", upload.status);
        }
        delay(0);
    });
    m_server->on("/ajax/config.json", HTTP_GET, [=]() {
        Serial.println("GET /ajax/config.json");
        DynamicJsonBuffer jsonBuffer;
        JsonObject& root = jsonBuffer.createObject();
        JsonArray& gpioArray = root.createNestedArray("gpios");
#ifdef ARDUINO_ARCH_ESP32
        const uint8_t maxGpio = 17;
        const uint8_t gpios[maxGpio] = {2, 4, 5, 12, 13, 14, 15, 16, 17, 18, 19, 23, 25, 26, 27, 32, 33};
#else    
        const uint8_t maxGpio = 11;
        const uint8_t gpios[maxGpio] = {0, 1, 2, 3, 4, 5, 12, 13, 14, 15, 16};
#endif    
        for (uint8_t idx = 0; idx < maxGpio; idx++)
            gpioArray.add(gpios[idx]);        
        
        JsonArray& json = root.createNestedArray("entries");
        auto cfgEntries = m_config->cfgEntries();
        HSDConfig::Group lastGroup = HSDConfig::Group::__Last;
        
        JsonArray* entries = nullptr;
        for (unsigned int index = 0; index < cfgEntries.size(); index++) {
            auto entry = cfgEntries[index];
            if (entry->type != HSDConfig::DataType::ColorMapping && 
                entry->type != HSDConfig::DataType::DeviceMapping) {
                if (lastGroup != entry->group) {
                    lastGroup = entry->group;
                    JsonObject& obj = json.createNestedObject();
                    obj["name"] = m_config->groupDescription(entry->group);
                    entries = &(obj.createNestedArray("entries"));
                }
                JsonObject& obj = entries->createNestedObject();
                obj["key"] = entry->key;
                obj["label"] = entry->label;
                if (entry->pattern.length() > 0)
                    obj["pattern"] = entry->pattern;
                if (entry->patternMsg.length() > 0)
                    obj["patternMsg"] = entry->patternMsg;
                if (entry->maxVal > 0)
                    obj["maxVal"] = entry->maxVal;
                obj["type"] = static_cast<int>(entry->type);
                switch (entry->type) {
                    case HSDConfig::DataType::String:
                    case HSDConfig::DataType::Password:
                        obj["value"] = *entry->value.string;
                        break;
                        
                    case HSDConfig::DataType::Bool:
                        obj["value"] = *entry->value.boolean;
                        break;
                        
                    case HSDConfig::DataType::Gpio:
                    case HSDConfig::DataType::Slider:
                        obj["value"] = *entry->value.byte;
                        break;
                        
                    case HSDConfig::DataType::Word:
                        obj["value"] = *entry->value.word;
                        break;

                    case HSDConfig::DataType::ColorMapping:
                    case HSDConfig::DataType::DeviceMapping:
                        break;
                }
            }
        }
        
        String jsonStr;
        root.printTo(jsonStr);
        m_server->send(200, "text/json;charset=utf-8", jsonStr);
    });
    m_server->on("/ajax/colormapping.json", HTTP_GET, [=]() {
        Serial.println("GET /ajax/colormapping.json");
        auto colMap = m_config->getColorMap();
        DynamicJsonBuffer jsonBuffer;
        JsonArray& colMapping = jsonBuffer.createArray();
        for (unsigned int index = 0; index < colMap.size(); index++) {
            JsonObject& colorMappingEntry = colMapping.createNestedObject(); 
            auto mapping = colMap[index];
            colorMappingEntry["id"] = index;
            colorMappingEntry["msg"] = mapping->msg;
            colorMappingEntry["col"] = m_config->hex2string(mapping->color);
            colorMappingEntry["beh"] = static_cast<int>(mapping->behavior);
        }
        String json;
        colMapping.printTo(json);
        m_server->send(200, "text/json;charset=utf-8", json);
    });
    m_server->on("/ajax/devicemapping.json", HTTP_GET, [=]() {
        Serial.println("GET /ajax/devicemapping.json");
        auto devMap = m_config->getDeviceMap();
        DynamicJsonBuffer jsonBuffer;
        JsonArray& devMapping = jsonBuffer.createArray();
        for (unsigned int index = 0; index < devMap.size(); index++) {
            JsonObject& deviceMappingEntry = devMapping.createNestedObject(); 
            auto mapping = devMap[index];
            deviceMappingEntry["id"] = index;
            deviceMappingEntry["device"] = mapping->device;
            deviceMappingEntry["led"] = mapping->ledNumber;
        }
        String json;
        devMapping.printTo(json);
        m_server->send(200, "text/json;charset=utf-8", json);
    });
    m_server->on("/ajax/status.json", HTTP_GET, [=]() {
        Serial.println("GET /ajax/status.json");
        DynamicJsonBuffer jsonBuffer;
        JsonObject& rootObj = jsonBuffer.createObject();
        JsonArray& types = rootObj.createNestedArray("table");
        StatusClass type = StatusClass::__Last;
        JsonArray* entries = nullptr;
        for (unsigned int index = 0; index < m_statusEntries.size(); index++) {
            auto entry = m_statusEntries.at(index);
            if (entry->type != type) {
                type = entry->type;
                JsonObject& typeObj = types.createNestedObject();
                typeObj["name"] = getTypeName(entry->type);
                entries = &(typeObj.createNestedArray("entries"));
            }
            JsonObject& obj = entries->createNestedObject();
            obj["label"] = entry->label;
            obj["value"] = entry->value;
            if (entry->unit.length() > 0)
                obj["unit"] = entry->unit;
            if (entry->id.length() > 0)
                obj["id"] = entry->id;
        }
        JsonArray& leds = rootObj.createNestedArray("leds");
        createLedArray(leds);
        
        String jsonStr;
        rootObj.printTo(jsonStr);
        m_server->send(200, "text/json;charset=utf-8", jsonStr);
    });
    m_server->onNotFound(std::bind(&HSDWebserver::deliverNotFoundPage, this));
    m_server->begin();
    m_ws->begin();
    
    unsigned long uptime(0);
    char buffer[64];
    m_statusEntries.push_back(new StatusEntry(StatusClass::Firmware, "Version", "V" HSD_VERSION));
    m_statusEntries.push_back(new StatusEntry(StatusClass::Firmware, "Build date", __DATE__ " " __TIME__));
#ifdef ESP32
    m_statusEntries.push_back(new StatusEntry(StatusClass::Firmware, "SDK-Version", ESP.getSdkVersion()));
    snprintf(buffer, 64, "ESP32 (Rev. %d) - %d MHz", ESP.getChipRevision(), ESP.getCpuFreqMHz());
#elif defined(ESP8266)
    m_statusEntries.push_back(new StatusEntry(StatusClass::Firmware, "SDK-Version", ESP.getCoreVersion() + " / " + String(ESP.getSdkVersion())));
    snprintf(buffer, 64, "ESP8266 - %d MHz", ESP.getCpuFreqMHz());
#endif
    m_statusEntries.push_back(new StatusEntry(StatusClass::Device, "CPU", buffer));
    m_statusEntries.push_back(new StatusEntry(StatusClass::Device, "Uptime", getUptimeString(uptime), "", "uptime"));
#ifdef ESP8266
    m_statusEntries.push_back(new StatusEntry(StatusClass::Device, "Voltage", String(ESP.getVcc()), "mV", "voltage"));
    snprintf(buffer, 64, "%08X", ESP.getChipId());
    m_statusEntries.push_back(new StatusEntry(StatusClass::Device, "ESP Chip ID", buffer));
#elif defined(ESP32)
    m_statusEntries.push_back(new StatusEntry(StatusClass::Heap, "Total size", String(ESP.getHeapSize()), "Bytes"));
#endif
    m_statusEntries.push_back(new StatusEntry(StatusClass::Heap, "Free", String(ESP.getFreeHeap()), "Bytes", "freeHeap"));
#ifdef ESP8266    
    m_statusEntries.push_back(new StatusEntry(StatusClass::Heap, "Max free block size", String(ESP.getMaxFreeBlockSize()), "Bytes", "maxFreeBlock"));
    m_statusEntries.push_back(new StatusEntry(StatusClass::Heap, "Fragmentation", String(ESP.getHeapFragmentation()), "%", "fragmentation"));
#elif defined(ESP32)
    m_statusEntries.push_back(new StatusEntry(StatusClass::Heap, "Lowest level of free heap since boot", String(ESP.getMinFreeHeap()), "Bytes", "minFree"));
    m_statusEntries.push_back(new StatusEntry(StatusClass::Heap, "Largest block of heap that can be allocated at once", String(ESP.getMaxAllocHeap()), "Bytes", "maxAllocHeap"));
#endif
    m_statusEntries.push_back(new StatusEntry(StatusClass::Network, "SSID", WiFi.SSID(), "", "ssid"));
    m_statusEntries.push_back(new StatusEntry(StatusClass::Network, "BSSID", WiFi.BSSIDstr(), "", "bssid"));
    m_statusEntries.push_back(new StatusEntry(StatusClass::Network, "Channel", String(WiFi.channel()), "", "channel"));
    m_statusEntries.push_back(new StatusEntry(StatusClass::Network, "Rssi", String(WiFi.RSSI()), "dBm", "rssi"));
    m_statusEntries.push_back(new StatusEntry(StatusClass::Network, "MAC address", WiFi.macAddress()));
    m_statusEntries.push_back(new StatusEntry(StatusClass::Network, "IP address", WiFi.localIP().toString(), "", "ip"));
    m_statusEntries.push_back(new StatusEntry(StatusClass::Network, "Subnet Mask", WiFi.subnetMask().toString(), "", "subnetMask"));
    m_statusEntries.push_back(new StatusEntry(StatusClass::Network, "Gateway", WiFi.gatewayIP().toString(), "", "gateway"));
    m_statusEntries.push_back(new StatusEntry(StatusClass::Mqtt, "Server", m_config->getMqttServer() + ":" + String(m_config->getMqttPort())));
    m_statusEntries.push_back(new StatusEntry(StatusClass::Mqtt, "Status", m_mqtt->connected() ? "Connected" : "Disconnected", "", "mqttStatus"));
#ifdef ESP8266
    snprintf(buffer, 64, "%08X", ESP.getFlashChipId());
    m_statusEntries.push_back(new StatusEntry(StatusClass::Flash, "Chip ID", buffer));
#endif
    m_statusEntries.push_back(new StatusEntry(StatusClass::Flash, "Sketch thinks Flash RAM is size", String(ESP.getFlashChipSize() / 1024), "KB"));
#ifdef ESP8266
    m_statusEntries.push_back(new StatusEntry(StatusClass::Flash, "Actual size based on chip Id", String(ESP.getFlashChipRealSize() / 1024), "KB")); 
#endif
    m_statusEntries.push_back(new StatusEntry(StatusClass::Flash, "Used flash memory", String(ESP.getSketchSize() / 1024), "KB")); 
    m_statusEntries.push_back(new StatusEntry(StatusClass::Flash, "Free flash memory", String(ESP.getFreeSketchSpace() / 1024), "KB")); 
    m_statusEntries.push_back(new StatusEntry(StatusClass::Flash, "Flash frequency", String(ESP.getFlashChipSpeed() / 1000 / 1000), "MHz")); 
    FlashMode_t ideMode = ESP.getFlashChipMode();
    String s = ideMode == FM_QIO ? "QIO" : ideMode == FM_QOUT ? "QOUT" : ideMode == FM_DIO ? "DIO" : ideMode == FM_DOUT ? "DOUT" : "UNKNOWN";
    m_statusEntries.push_back(new StatusEntry(StatusClass::Flash, "Flash write mode", s));
#ifdef ESP8266
    FSInfo fsInfo;
    SPIFFS.info(fsInfo);
    m_statusEntries.push_back(new StatusEntry(StatusClass::Filesystem, "Total KB", String(fsInfo.totalBytes / 1024.0), "KB"));
    m_statusEntries.push_back(new StatusEntry(StatusClass::Filesystem, "Used KB", String(fsInfo.usedBytes / 1024.0), "KB"));
    m_statusEntries.push_back(new StatusEntry(StatusClass::Filesystem, "Block size", String(fsInfo.blockSize)));
    m_statusEntries.push_back(new StatusEntry(StatusClass::Filesystem, "Page size", String(fsInfo.pageSize)));
    m_statusEntries.push_back(new StatusEntry(StatusClass::Filesystem, "Maximum open files", String(fsInfo.maxOpenFiles)));
    m_statusEntries.push_back(new StatusEntry(StatusClass::Filesystem, "Maximum path length", String(fsInfo.maxPathLength)));
#elif defined(ESP32)    
    m_statusEntries.push_back(new StatusEntry(StatusClass::Filesystem, "Total KB", String(SPIFFS.totalBytes() / 1024.0), "KB"));
    m_statusEntries.push_back(new StatusEntry(StatusClass::Filesystem, "Used KB", String(SPIFFS.usedBytes() / 1024.0), "KB"));
#endif    
}

// ---------------------------------------------------------------------------------------------------------------------

void HSDWebserver::updateStatusEntry(const String& id, const String& value) {
    for (size_t idx = 0; idx < m_statusEntries.size(); idx++)
        if (m_statusEntries[idx]->id == id)
            m_statusEntries[idx]->value = value;
}

// ---------------------------------------------------------------------------------------------------------------------

void HSDWebserver::setUptime(unsigned long& deviceUptime) {
    Serial.printf("setUptime(%lu) - ", deviceUptime);
    updateStatusEntry("rssi", String(WiFi.RSSI()));
    updateStatusEntry("uptime", getUptimeString(deviceUptime));
#ifdef ESP32
    updateStatusEntry("freeHeap", String(ESP.getFreeHeap()));
    updateStatusEntry("maxAllocHeap", String(ESP.getMaxAllocHeap()));
    updateStatusEntry("minFree", String(ESP.getMinFreeHeap()));
#endif        
#ifdef ESP8266
    updateStatusEntry("fragmentation", String(ESP.getHeapFragmentation()));
    updateStatusEntry("freeHeap", String(ESP.getFreeHeap()));
    updateStatusEntry("maxFreeBlock", String(ESP.getMaxFreeBlockSize()));
    updateStatusEntry("voltage", String(ESP.getVcc()));
#endif
    
    if (m_ws->connectedClients()) {
        String res = createUpdateRequest();
        Serial.printf("%u client: '%s'\n", m_ws->connectedClients(), res.c_str());
        m_ws->broadcastTXT(res);
    } else {
        Serial.println("no websocket connected");
    }
}

// ---------------------------------------------------------------------------------------------------------------------

void HSDWebserver::createLedArray(JsonArray& leds) const {
    uint8_t idx(0);
    for (int ledNr = 0; ledNr < m_config->getNumberOfLeds(); ledNr++) {
        uint32_t color = m_leds->getColor(ledNr);
        HSDConfig::Behavior behavior = m_leds->getBehavior(ledNr);
        if ((LED_COLOR_NONE != color) && (HSDConfig::Behavior::Off != behavior)) {
            JsonObject& ledObj = leds.createNestedObject();
            ledObj["id"] = idx++;
            ledObj["led"] = ledNr;
            ledObj["device"] = m_config->getDevice(ledNr);
            ledObj["col"] = m_config->hex2string(color);
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
        if (m_statusEntries[idx]->id.length() > 0)
            fields[m_statusEntries[idx]->id] = m_statusEntries[idx]->value;
       
    String res;
    json.printTo(res);
    return res;
}

// ---------------------------------------------------------------------------------------------------------------------

void HSDWebserver::deliverNotFoundPage() {
    if (!handleFileRead(m_server->uri())) {
        Serial.print("File not found: ");
        Serial.println(m_server->uri());
        String html = "File Not Found\n\nURI: ";
        html += m_server->uri();
        html += "\nMethod: ";
        html += m_server->method() == HTTP_GET ? "GET" : "POST";
        html += "\nArguments: ";
        html += m_server->args();
        html += "\n";
        for (uint8_t i = 0; i < m_server->args(); i++)
            html += " " + m_server->argName(i) + ": " + m_server->arg(i) + "\n";
        m_server->send(404, "text/plain", html);
    }
}

// ---------------------------------------------------------------------------------------------------------------------

String HSDWebserver::getConfig() const {
    String tabs = "\n              <div class=\"mdl-tabs__tab-bar\">\n";
    String form = "\n              <form id=\"formConfig\">";
    auto entries = m_config->cfgEntries();
    HSDConfig::Group lastGroup = HSDConfig::Group::__Last;
    for (size_t idx = 0; idx < entries.size(); idx++) {
        auto entry = entries[idx];
        if (lastGroup != entry->group) {
            String groupName = m_config->groupDescription(entry->group);
            lastGroup = entry->group;
            tabs += "                <a href=\"#tab-cfg";
            tabs += groupName;
            tabs += "\" class=\"mdl-tabs__tab";
            if (idx == 0)
                tabs += " is-active";
            tabs += "\">";
            tabs += groupName;
            tabs += "</a>\n";
            
            form += "\n                <div class=\"mdl-tabs__panel";
            if (idx == 0)
                form += " is-active";
            form += "\" id=\"tab-cfg";
            form += groupName;
            form += "\"></div>";
        }
    }
    tabs += "              </div>\n";
    form += "\n              </form>";
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

bool HSDWebserver::handleFileRead(String path) {
    String filepath;
    if (SPIFFS.exists(path)) { 
        Serial.println("handleFileRead: " + path);
        filepath = path;
    } else if (SPIFFS.exists(path + ".gz")) {
        Serial.println("handleFileRead: " + path + ".gz");
        // m_server.sendHeader("Content-Encoding", "gzip"); // automatically added by the framework
        filepath = path + ".gz";
    }   

    if (filepath.length() > 0) {
        String contentType;
        if (path.endsWith(".json")) {
            m_server->sendHeader("Content-Disposition", "attachment; filename=" + (path.lastIndexOf('/') == -1 ? path : path.substring(path.lastIndexOf('/') + 1)));
            contentType = "application/octet-stream;charset=utf-8";
        } else {
#ifdef ESP8266            
            contentType = esp8266webserver::StaticRequestHandler<WiFiServer>::getContentType(path);
#elif defined ESP32
            contentType = StaticRequestHandler::getContentType(path);
#endif            
        }
//        m_server.sendHeader("Cache-Control", "max-age=86400");
        File file = SPIFFS.open(filepath, "r");
        m_server->streamFile(file, contentType);
        file.close();
        return true;
    }
    return false;    
}

// ---------------------------------------------------------------------------------------------------------------------

void HSDWebserver::handleWebSocket(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
    bool msgReceived(false);
    if (type == WStype_CONNECTED) {
        Serial.printf("ws[%u] connect from %s\n", num, m_ws->remoteIP(num).toString().c_str());
        m_ws->sendPing(num);
        String payload = createUpdateRequest();
        m_ws->sendTXT(num, payload);
    } else if (type == WStype_DISCONNECTED) {
        Serial.printf("ws[%u] disconnect\n", num);
    } else if (type == WStype_ERROR) {
        Serial.printf("ws[%u] error: %s\n", num, length ? reinterpret_cast<const char*>(payload): "");
    } else if (type == WStype_PONG) {
        if (length)
            Serial.printf("ws[%u] pong[%u]: %s\n", num, length, reinterpret_cast<const char*>(payload));
        else
            Serial.printf("ws[%u] pong[%u]\n", num, length);
    } else if (type == WStype_TEXT) {
        Serial.printf("ws[%u] text received: %s\n", num, reinterpret_cast<const char*>(payload));
        m_wsBuffer[num] = reinterpret_cast<const char*>(payload);
        msgReceived = true;
    } else if (type == WStype_FRAGMENT_TEXT_START) {
        Serial.printf("ws[%u] text fragment start: %s\n", num, reinterpret_cast<const char*>(payload));
        m_wsBuffer[num] = reinterpret_cast<const char*>(payload);
    } else if (type == WStype_FRAGMENT) {
        Serial.printf("ws[%u] text fragment: %s\n", num, reinterpret_cast<const char*>(payload));
        m_wsBuffer[num] += reinterpret_cast<const char*>(payload);
    } else if (type == WStype_FRAGMENT_FIN) {
        Serial.printf("ws[%u] text fragment: %s\n", num, reinterpret_cast<const char*>(payload));
        m_wsBuffer[num] += reinterpret_cast<const char*>(payload);
        msgReceived = true;
    } else {
        Serial.printf("ws[%u] - event %u", num, type);
    }
    
    if (msgReceived) {
        DynamicJsonBuffer jsonBuffer(m_wsBuffer[num].length() + 1);
        JsonObject& reqObj = jsonBuffer.parseObject(m_wsBuffer[num]);
        String method = reqObj["method"];
        if (method == "importCfg") {
            importConfig(reqObj["filename"], reqObj["data"]);
        } else if (method == "reboot") {
            Serial.println("Rebooting ESP...");
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
        m_wsBuffer[num] = ""; // cleanup memory
    }
}

// ---------------------------------------------------------------------------------------------------------------------

void HSDWebserver::importConfig(const String& filename, const String& content) const {
    Serial.printf("Import config: %s\n", filename.c_str());
    File file = SPIFFS.open(FILENAME_MAINCONFIG, "w+");
    if (!file) {
        Serial.printf("Failed to open %s\n", FILENAME_MAINCONFIG);
    } else {
#ifdef ESP8266
        if (file.write(content.c_str()) != content.length())
#elif defined(ESP32)
        if (file.write(reinterpret_cast<const uint8_t*>(content.c_str()), content.length()) != content.length())
#endif            
            Serial.println("Failed to write file");
        file.close();
        m_config->readConfigFile();
    }
}    

// ---------------------------------------------------------------------------------------------------------------------

void HSDWebserver::ledChange() {
    if (m_ws->connectedClients()) {
        DynamicJsonBuffer jsonBuffer;
        JsonObject& json = jsonBuffer.createObject();
        json["method"] = "updLeds";
        createLedArray(json.createNestedArray("data"));
    
        String res;
        json.printTo(res);
        m_ws->broadcastTXT(res);
    }
}

// ---------------------------------------------------------------------------------------------------------------------

String HSDWebserver::processTemplates(const String& key) const {
    Serial.printf("processTemplates(%s)\n", key.c_str());
    if (key == "CONFIG") {
        return getConfig();
    } else if (key == "VERSION") {
        return "V" HSD_VERSION;
    } else {
        Serial.println("UNKNOWN KEY");
        return "";
    }
}

// ---------------------------------------------------------------------------------------------------------------------

void HSDWebserver::saveColorMapping(const JsonArray& colMapping) const {
    Serial.print("Received colormapping: ");
    colMapping.printTo(Serial);
    Serial.println("");
    vector<HSDConfig::ColorMapping*> colMap;
    for (size_t i = 0; i < colMapping.size(); i++) {
        const JsonObject& elem = colMapping.get<JsonVariant>(i).as<JsonObject>();
        colMap.push_back(new HSDConfig::ColorMapping(elem["msg"].as<String>(), 
                                                     m_config->string2hex(elem["col"].as<String>()), 
                                                     static_cast<HSDConfig::Behavior>(elem["beh"].is<int>() ? elem["beh"].as<int>() : elem["beh"].as<String>().toInt())));
    }
    m_config->setColorMap(colMap);
    m_config->writeConfigFile();
}

// ---------------------------------------------------------------------------------------------------------------------

void HSDWebserver::saveConfig(const JsonObject& config) const {
    Serial.print("Received configuration: ");
    config.printTo(Serial);
    Serial.println("");
    
    auto entries = m_config->cfgEntries();
    Serial.printf("Config has %u entries\n", entries.size());
    bool needSave(false);
    String key;
    for (size_t idx = 0; idx < entries.size(); idx++) {
        auto entry = entries[idx];
        if (entry->type == HSDConfig::DataType::ColorMapping || entry->type == HSDConfig::DataType::DeviceMapping)
            continue;
        key = m_config->groupDescription(entry->group) + "." + entry->key;
        Serial.printf("Checking config entry %d - key: %s\n", idx, key.c_str());
        if (!config.containsKey(key)) {
            Serial.printf("Missing key in configuration: %s\n", key.c_str());
        } else {
            switch (entry->type) {
                case HSDConfig::DataType::String:
                case HSDConfig::DataType::Password:
                    if (*entry->value.string != config[key].as<String>()) {
                        needSave = true;
                        *entry->value.string = config[key].as<String>();
                    }
                    break;

                case HSDConfig::DataType::Gpio:
                case HSDConfig::DataType::Slider:
                    if (*entry->value.byte != config[key].as<unsigned char>()) {
                        needSave = true;
                        *entry->value.byte = config[key].as<unsigned char>();
                    }
                    break;

                case HSDConfig::DataType::Word:
                    if (*entry->value.word != config[key].as<unsigned short>()) {
                        needSave = true;
                        *entry->value.word = config[key].as<unsigned short>();
                    }
                    break;

                case HSDConfig::DataType::Bool:
                    if (*entry->value.boolean != config[key].as<bool>()) {
                        needSave = true;
                        *entry->value.boolean = config[key].as<bool>();
                    }
                    break;

                case HSDConfig::DataType::ColorMapping:
                case HSDConfig::DataType::DeviceMapping:
                    break;
            }            
        }
    }
    if (needSave) {
        Serial.println("Main config has changed, storing it.");
        m_config->writeConfigFile();
    }
}

// ---------------------------------------------------------------------------------------------------------------------

void HSDWebserver::saveDeviceMapping(const JsonArray& devMapping) const {
    Serial.print("Received devicemapping: ");
    devMapping.printTo(Serial);
    Serial.println("");
    vector<HSDConfig::DeviceMapping*> devMap;
    for (size_t i = 0; i < devMapping.size(); i++) {
        const JsonObject& elem = devMapping.get<JsonVariant>(i).as<JsonObject>();
        devMap.push_back(new HSDConfig::DeviceMapping(elem["device"].as<String>(), 
                                                      elem["led"].is<int>() ? elem["led"].as<int>() : elem["led"].as<String>().toInt()));
    }
    m_config->setDeviceMap(devMap);
    m_config->writeConfigFile();
}

// ---------------------------------------------------------------------------------------------------------------------

void HSDWebserver::sendAndProcessTemplate(const String& filePath) {
    if (!SPIFFS.exists(filePath)) {
        Serial.printf("Cannot process %s: file does not exist\n", filePath.c_str());
        deliverNotFoundPage();
    } else {
        File file = SPIFFS.open(filePath, "r");
        if (!file) {
            Serial.printf("Cannot process %s: file does not exist\n", filePath.c_str());
            deliverNotFoundPage();
        } else {
            m_server->setContentLength(CONTENT_LENGTH_UNKNOWN); // Chunked transfer
            m_server->sendHeader("Content-Type", "text/html", true);
            m_server->sendHeader("Cache-Control", "no-cache");
            m_server->send(200);
            
            // Process!
            static const uint16_t MAX = 2048;
            String buffer, keyBuffer;
            buffer.reserve(MAX);
            int val;
            char ch;

            while ((val = file.read()) != -1) {
                ch = char(val);
                if (ch == '%') {
                    // Clear out buffer.
                    m_server->sendContent(buffer);
                    buffer = "";
                    buffer.reserve(MAX);

                    // Process substitution.
                    keyBuffer = "";
                    bool found(false);
                    while (!found && (val = file.read()) != -1) {
                        ch = char(val);
                        if (ch == '%')
                            found = true;
                        else
                            keyBuffer += ch;
                    }          
                    
                    if (val == -1 && !found) // Check for bad exit.
                        Serial.printf("Cannot process %s: unable to parse\n", filePath.c_str());

                    // Get substitution
                    String processed = processTemplates(keyBuffer);
                    // Serial.printf("Lookup '%s' received: %s\n", keyBuffer.c_str(), processed.c_str());
                    if (processed.length() < MAX)
                        buffer = processed;
                    else
                        m_server->sendContent(processed);
                } else {
                    buffer += ch;
                    if (buffer.length() >= MAX) {
                        m_server->sendContent(buffer);
                        buffer = "";
                        buffer.reserve(MAX);
                    }
                }
            }

            if (val == -1) {
                if (buffer.length() > 0)
                    m_server->sendContent(buffer);
                m_server->sendContent("");
            }            
            file.close();
        }
    }
}

// ---------------------------------------------------------------------------------------------------------------------

void HSDWebserver::setUpdaterError() {
    Update.printError(Serial);
    StreamString str;
    Update.printError(str);
    m_updaterError = str.c_str();
}
