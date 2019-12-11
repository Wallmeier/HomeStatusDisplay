#include "HSDWebserver.hpp"

#include <StreamString.h>
#ifdef ARDUINO_ARCH_ESP32
#include <Update.h>
#else
extern "C" uint32_t _FS_start;
extern "C" uint32_t _FS_end;
#include <Updater.h>
#endif // ARDUINO_ARCH_ESP32



HSDWebserver::HSDWebserver(HSDConfig& config, const HSDLeds& leds, const HSDMqtt& mqtt) :
    m_config(config),
    m_deviceUptimeMinutes(0),
    m_leds(leds),
    m_mqtt(mqtt),
    m_server(80)
{
}

// ---------------------------------------------------------------------------------------------------------------------

void HSDWebserver::begin() {
    Serial.println(F("Starting WebServer"));
    m_server.on("/", std::bind(&HSDWebserver::deliverStatusPage, this));
    m_server.on("/cfgmain", std::bind(&HSDWebserver::deliverConfigPage, this));
    m_server.on("/cfgcolormapping", std::bind(&HSDWebserver::deliverColorMappingPage, this));
    m_server.on("/cfgdevicemapping", std::bind(&HSDWebserver::deliverDeviceMappingPage, this));
    m_server.on("/cfgImport", HTTP_GET, [=]() {
        m_server.setContentLength(CONTENT_LENGTH_UNKNOWN);
        m_server.send(200);
        sendHeader("Config Import");
        m_server.sendContent(F("\t<form method='POST' action='/cfgImport' enctype='multipart/form-data'>\n\t\t<input type='file' name='import'>\n\t\t<input type='submit' value='Import'>\n\t</form>\n</body>"));
        m_server.sendContent("");
    });
    m_server.on("/cfgImport", HTTP_POST, std::bind(&HSDWebserver::deliverConfigPage, this), [=]() {
        HTTPUpload& upload = m_server.upload();
        if (upload.status == UPLOAD_FILE_START) {
            Serial.setDebugOutput(true);
            Serial.printf("Import config: %s\r\n", upload.filename.c_str());
            m_file = SPIFFS.open(FILENAME_MAINCONFIG, "w+");
            if (!m_file)
                Serial.printf("Failed to open %s\r\n", FILENAME_MAINCONFIG);
        } else if (upload.status == UPLOAD_FILE_WRITE) {
            if (m_file && m_file.write(upload.buf, upload.currentSize) != upload.currentSize)
                Serial.println("Failed to write file");
        } else if (upload.status == UPLOAD_FILE_END) {
            if (m_file) {
                m_file.close();
                m_config.readConfigFile();
            }
            Serial.setDebugOutput(false);
        } else {
            Serial.printf("Config Import Failed Unexpectedly (likely broken connection): status=%d\n", upload.status);
        }
    });
    m_server.on("/reboot", [=]() {
        Serial.println(F("Rebooting ESP..."));
        ESP.restart();
    });
    m_server.on("/colormapping.json", HTTP_GET, [=]() {
        const QList<HSDConfig::ColorMapping>& colMap = m_config.getColorMap();
        DynamicJsonBuffer jsonBuffer;
        JsonArray& colMapping = jsonBuffer.createArray();
        for (unsigned int index = 0; index < colMap.size(); index++) {
            JsonObject& colorMappingEntry = colMapping.createNestedObject(); 
            const HSDConfig::ColorMapping& mapping = colMap[index];
            colorMappingEntry["id"] = index;
            colorMappingEntry["msg"] = mapping.msg;
            colorMappingEntry["col"] = m_config.hex2string(mapping.color);
            colorMappingEntry["beh"] = static_cast<int>(mapping.behavior);
        }
        String json;
        colMapping.printTo(json);
        m_server.send(200, "text/json;charset=utf-8", json);
    });
    m_server.on("/colormapping.json", HTTP_PUT, [=]() {
        if (m_server.hasArg("plain")) {
            String data = m_server.arg("plain");
            m_server.send(200);
            Serial.print("Received colormapping.json: ");
            Serial.println(data);
            DynamicJsonBuffer jsonBuffer(data.length() + 1);
            JsonArray& colMapping = jsonBuffer.parseArray(data);
            QList<HSDConfig::ColorMapping> colMap;
            for (size_t i = 0; i < colMapping.size(); i++) {
                const JsonObject& elem = colMapping.get<JsonVariant>(i).as<JsonObject>();
                colMap.push_back(HSDConfig::ColorMapping(elem["msg"].as<String>(), 
                                                         m_config.string2hex(elem["col"].as<String>()), 
                                                         static_cast<HSDConfig::Behavior>(elem["beh"].is<int>() ? elem["beh"].as<int>() : elem["beh"].as<String>().toInt())));
            }
            m_config.setColorMap(colMap);
            m_config.writeConfigFile();
        } else {
            Serial.println(F("Bad request - no content for /colormapping.json"));
            m_server.send(500);
        }
    });
    m_server.on("/devicemapping.json", HTTP_GET, [=]() {
        const QList<HSDConfig::DeviceMapping>& devMap = m_config.getDeviceMap();
        DynamicJsonBuffer jsonBuffer;
        JsonArray& devMapping = jsonBuffer.createArray();
        for (unsigned int index = 0; index < devMap.size(); index++) {
            JsonObject& deviceMappingEntry = devMapping.createNestedObject(); 
            const HSDConfig::DeviceMapping& mapping = devMap[index];
            deviceMappingEntry["id"] = index;
            deviceMappingEntry["device"] = mapping.device;
            deviceMappingEntry["led"] = mapping.ledNumber;
        }
        String json;
        devMapping.printTo(json);
        m_server.send(200, "text/json;charset=utf-8", json);
    });
    m_server.on("/devicemapping.json", HTTP_PUT, [=]() {
        if (m_server.hasArg("plain")) {
            String data = m_server.arg("plain");
            m_server.send(200);
            Serial.print("Received devicemapping.json: ");
            Serial.println(data);
            DynamicJsonBuffer jsonBuffer(data.length() + 1);
            JsonArray& devMapping = jsonBuffer.parseArray(data);
            QList<HSDConfig::DeviceMapping> devMap;
            for (size_t i = 0; i < devMapping.size(); i++) {
                const JsonObject& elem = devMapping.get<JsonVariant>(i).as<JsonObject>();
                devMap.push_back(HSDConfig::DeviceMapping(elem["device"].as<String>(), 
                                                          elem["led"].is<int>() ? elem["led"].as<int>() : elem["led"].as<String>().toInt()));
            }
            m_config.setDeviceMap(devMap);
            m_config.writeConfigFile();
        } else {
            Serial.println(F("Bad request - no content for /devicemapping.json"));
            m_server.send(500);
        }
    });
    m_server.on("/update", HTTP_GET, [=]() {
        m_server.setContentLength(CONTENT_LENGTH_UNKNOWN);
        m_server.send(200);
        sendHeader("Update");
        m_server.sendContent(F("\t<form method='POST' action='/update' enctype='multipart/form-data'>\n"
                               "\t\tFirmware:<br>\n"
                               "\t\t<input type='file' size='40' accept='.bin' name='firmware'>\n"
                               "\t\t<input type='submit' value='Update Firmware'>\n"
                               "\t</form><br>\n"
                               "\t\tFileSystem:<br>\n"
                               "\t\t<input type='file' size='40' accept='.bin' name='filesystem'>\n"
                               "\t\t<input type='submit' value='Update FileSystem'>\n"
                               "\t</form>\n"
                               "</body>"));
        m_server.sendContent("");
    });
    m_server.on("/update", HTTP_POST, [=]() {
        if (Update.hasError()) {
            m_server.send(200, F("text/html"), String(F("Update error: ")) + m_updaterError);
        } else {
            m_server.client().setNoDelay(true);
            m_server.send(200, F("text/html"), F("<META http-equiv=\"refresh\" content=\"15;URL=/\">Update Success! Rebooting..."));
            delay(100);
            m_server.client().stop();
            ESP.restart();
        }
    }, [=]() {
        HTTPUpload& upload = m_server.upload();
        if (upload.status == UPLOAD_FILE_START) {
            m_updaterError = String();
            Serial.setDebugOutput(true);
            Serial.printf("Update: %s\r\n", upload.filename.c_str());
            
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
                Serial.printf("Update Success: %u\r\nRebooting...\r\n", upload.totalSize);
            else
                setUpdaterError();
            Serial.setDebugOutput(false);
        } else if (upload.status == UPLOAD_FILE_ABORTED){
            Update.end();
            Serial.println("Update was aborted");
        } else {
            Serial.printf("Update Failed Unexpectedly (likely broken connection): status=%d\r\n", upload.status);
        }
        delay(0);
    });
    m_server.onNotFound(std::bind(&HSDWebserver::deliverNotFoundPage, this));
    m_server.begin();
}

// ---------------------------------------------------------------------------------------------------------------------

void HSDWebserver::handleClient(unsigned long deviceUptime) {
    m_deviceUptimeMinutes = deviceUptime;
    m_server.handleClient();
}

// ---------------------------------------------------------------------------------------------------------------------

void HSDWebserver::deliverConfigPage() {
    String html;
    html.reserve(1000);

    bool needSave(updateMainConfig());

    m_server.setContentLength(CONTENT_LENGTH_UNKNOWN);
    m_server.send(200);
    sendHeader("General configuration");

    html = F("\t<form action=\"/cfgmain\">\n\t\t<table class=\"cfg\">\n");

    const QList<HSDConfig::ConfigEntry>& entries = m_config.cfgEntries();
    HSDConfig::Group lastGroup = HSDConfig::Group::__Last;
    for (size_t idx = 0; idx < entries.size(); idx++) {
        const HSDConfig::ConfigEntry& entry = entries[idx];
        if (entry.type == HSDConfig::DataType::ColorMapping || entry.type == HSDConfig::DataType::DeviceMapping)
            continue;
        if (entry.group != lastGroup) {
            lastGroup = entries[idx].group;
            m_server.sendContent(html);
            html  = F("\t\t\t<tr><td class=\"cfgTitle\" colspan=\"2\">");
            html += m_config.groupDescription(entry.group);
            html += F("</td></tr>\n");
        }
        html += F("\t\t\t<tr><td>");
        html += entry.label;
        html += F("</td><td><input name=\"");
        html += m_config.groupDescription(entry.group);
        html += F(".");
        html += entry.key;
        if (entries[idx].placeholder != nullptr) {
            html += F("\" placeholder=\"");
            html += entry.placeholder;
        }
        html += F("\" type=\"");
        switch (entry.type) {
            case HSDConfig::DataType::String:
            case HSDConfig::DataType::Password:
                html += entry.type == HSDConfig::DataType::Password ? F("password") : F("text");
                html += F("\" size=\"30\" value=\"");
                html += *entry.value.string;
                html += F("\">");
                break;

            case HSDConfig::DataType::Byte:
            case HSDConfig::DataType::Word:
                html += F("text\" size=\"30\" maxlength=\"");
                html += String(entry.maxLength);
                html += F("\" value=\"");
                if (entry.type == HSDConfig::DataType::Byte)
                    html += String(*entry.value.byte);
                else
                    html += String(*entry.value.word);
                html += F("\">");
                break;

            case HSDConfig::DataType::Bool:
                html += F("checkbox\"");
                if (*entry.value.boolean)
                    html += F(" checked");
                html += F(">");
                break;
        }
        html += F("</td></tr>\n");
    };

    html += F("\t\t</table>\n"
             "\t\t<p>\n"
             "\t\t\t<input type='submit' class='button' value='Save'>\n"
             "\t\t\t<input type='button' class='button' onclick=\"location.href='/config.json'\" value='Export'>\n"
             "\t\t\t<input type='button' class='button' onclick=\"location.href='./cfgImport'\" value='Import'>\n"
             "\t\t</p>\n"
             "\t</form>\n</body>\n</html>");
    m_server.sendContent(html);
    m_server.sendContent("");

    if (needSave) {
        Serial.println(F("Main config has changed, storing it."));
        m_config.writeConfigFile();
    }
    Serial.print(F("Free RAM: ")); Serial.println(ESP.getFreeHeap());
}

// ---------------------------------------------------------------------------------------------------------------------

void HSDWebserver::deliverStatusPage() {
    String html;
    html.reserve(1000);

    m_server.setContentLength(CONTENT_LENGTH_UNKNOWN);
    m_server.send(200);
    sendHeader("Status");

    html = F("<p>Device uptime: ");
    char buffer[50];
    memset(buffer, 0, sizeof(buffer));
    sprintf(buffer, "%lu days, %lu hours, %lu minutes", m_deviceUptimeMinutes / 1440, (m_deviceUptimeMinutes / 60) % 24,
            m_deviceUptimeMinutes % 60);
    html += String(buffer);
    html += F("</p>\n");

#ifdef ARDUINO_ARCH_ESP32
    html += F("<p>Device Heap stats (size, free, minFree, max) [Bytes]: ");
    html += String(ESP.getHeapSize()) + ", " + String(ESP.getFreeHeap()) + ", " + String(ESP.getMinFreeHeap()) + ", " + String(ESP.getMaxAllocHeap()) + "</p>\n";
#else
    uint32_t free;
    uint16_t max;
    uint8_t frag;
    ESP.getHeapStats(&free, &max, &frag);

    html += F("<p>Device RAM stats (free, max, frag) [Bytes]: ");
    html += String(free) + ", " + String(max) + ", " + String(frag);
    html += F("</p>\n"
              "<p>Device voltage: ");
    html += String(ESP.getVcc());
    html += F(" mV</p>\n");
#endif

#ifdef HSD_SENSOR_ENABLED
    if (m_config.getSensorSonoffEnabled() && m_sensorData.length() > 0) {
        html += F("<p>Sensor: ");
        html += m_sensorData;
        html += F("</p>\n");
    }
#endif // HSD_SENSOR_ENABLED

    if (WiFi.status() == WL_CONNECTED) {
        html += F("<p>Device is connected to WLAN <b>");
        html += WiFi.SSID();
        html += F("</b><br/>IP address is <b>");
        IPAddress ip = WiFi.localIP();
        char buffer[20];
        memset(buffer, 0, sizeof(buffer));
        sprintf(buffer, "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
        html += String(buffer);
        html += F("</b><br/>MAC address is <b>");
        html += WiFi.macAddress();
        html += F("</b></p>\n");
    } else {
        html += F("<p>Device is not connected to local network</p>\n");
    }

    if (m_mqtt.connected()) {
        html += F("<p>Device is connected to  MQTT broker <b>");
        html += m_config.getMqttServer();
        html += F("</b></p>\n");
    } else {
        html += F("<p>Device is not connected to an MQTT broker</p>\n");
    }
    m_server.sendContent(html);

    if (m_config.getNumberOfLeds() == 0) {
        html = F("<p>No LEDs configured yet</p>\n");
    } else {
        html = F("");
        int ledOnCount(0);
        for (int ledNr = 0; ledNr < m_config.getNumberOfLeds(); ledNr++) {
            uint32_t color = m_leds.getColor(ledNr);
            HSDConfig::Behavior behavior = m_leds.getBehavior(ledNr);
            String device = m_config.getDevice(ledNr);
            if ((m_config.getDefaultColor("NONE") != color) && (HSDConfig::Behavior::Off != behavior)) {
                String colorName = m_config.getDefaultColor(color);
                if (colorName == "")
                    colorName = m_config.hex2string(color);
                html += F("<p><div class='hsdcolor' style='background-color:");
                html += m_config.hex2string(color);
                html += F("';></div>&nbsp;");
                html += F("LED <b>");
                html += ledNr;
                html += F("</b> for <b>");
                html += device;
                html += F("</b> is <b>");
                html += behavior2String(behavior);
                html += F("</b> with color <b>");
                html += colorName;
                html += F("</b><br/></p>\n");

                ledOnCount++;
            }
        }

        if (ledOnCount == 0)
            html += F("<p>All LEDs are <b>off</b></p>\n");
    }
    html += F("</body></html>");
    m_server.sendContent(html);
    m_server.sendContent("");
}

// ---------------------------------------------------------------------------------------------------------------------

void HSDWebserver::deliverColorMappingPage() {
    m_server.setContentLength(CONTENT_LENGTH_UNKNOWN);
    m_server.send(200);
    sendHeader("Color mapping configuration", true);
    m_server.sendContent(F("\t<div id='colmap-table' class='table'></div>\n"
                           "\t<script type='text/javascript' src='js/colmap.js'></script>\n"
                           "\t<p>\n"
                           "\t\t<input type='button' class='button' onclick='var maxIdx=getMaxIdx(coltable); coltable.addData([{id:maxIdx, msg:\"\", col:\"#00FF00\", beh:1}], false)' value='Add Row'>\n"
                           "\t\t<input type='button' class='button' onclick='coltable.clearData()' value='Delete All'>\n"
                           "\t\t<input type='button' class='button' onclick='coltable.undo()' value='Undo'><br>\n"
                           "\t\t<input type='button' class='button' onclick='saveTable(coltable, \"/colormapping.json\")' value='Save'>\n"
                           "\t</p>\n"
                           "</body>\n</html>"));
    m_server.sendContent("");
    Serial.print(F("Free RAM: ")); Serial.println(ESP.getFreeHeap());
}

// ---------------------------------------------------------------------------------------------------------------------

void HSDWebserver::deliverDeviceMappingPage() {
    m_server.setContentLength(CONTENT_LENGTH_UNKNOWN);
    m_server.send(200);
    sendHeader("Device mapping configuration", true);
    m_server.sendContent(F("\t<div id='devmap-table' class='table'></div>\n"
                           "\t<script type='text/javascript' src='js/devmap.js'></script>\n"
                           "\t<p>\n"
                           "\t\t<input type='button' class='button' onclick='var maxIdx=getMaxIdx(devtable); devtable.addData([{id:maxIdx, device:\"\", led:0}], false)' value='Add Row'>\n"
                           "\t\t<input type='button' class='button' onclick='devtable.clearData()' value='Delete All'>\n"
                           "\t\t<input type='button' class='button' onclick='devtable.undo()' value='Undo'><br>\n"
                           "\t\t<input type='button' class='button' onclick='saveTable(devtable, \"/devicemapping.json\")' value='Save'>\n"
                           "\t</p>\n"
                           "</body>\n</html>"));
    m_server.sendContent("");
    Serial.print(F("Free RAM: ")); Serial.println(ESP.getFreeHeap());
}

// ---------------------------------------------------------------------------------------------------------------------

void HSDWebserver::deliverNotFoundPage() {
    if (!handleFileRead(m_server.uri())) {
        Serial.print("File not found: ");
        Serial.println(m_server.uri());
        String html = F("File Not Found\n\n"
                        "URI: ");
        html += m_server.uri();
        html += F("\nMethod: ");
        html += (m_server.method() == HTTP_GET) ? F("GET") : F("POST");
        html += F("\nArguments: ");
        html += m_server.args();
        html += F("\n");
        for (uint8_t i = 0; i < m_server.args(); i++)
            html += " " + m_server.argName(i) + ": " + m_server.arg(i) + "\n";
        m_server.send(404, F("text/plain"), html);
    }
}

// ---------------------------------------------------------------------------------------------------------------------

bool HSDWebserver::updateMainConfig() {
    bool needSave(false);
    const QList<HSDConfig::ConfigEntry>& entries = m_config.cfgEntries();
    String key = m_config.groupDescription(entries[0].group) + "." + entries[0].key;
    if (m_server.hasArg(key)) {
        for (size_t idx = 0; idx < entries.size(); idx++) {
            const HSDConfig::ConfigEntry& entry = entries[idx];
            if (entry.type == HSDConfig::DataType::ColorMapping || entry.type == HSDConfig::DataType::DeviceMapping)
                continue;
            key = m_config.groupDescription(entry.group) + "." + entry.key;
            switch (entries[idx].type) {
                case HSDConfig::DataType::String:
                case HSDConfig::DataType::Password:
                    if (*entry.value.string != m_server.arg(key)) {
                        needSave = true;
                        *entry.value.string = m_server.arg(key);
                    }
                    break;

                case HSDConfig::DataType::Byte:
                    if (*entry.value.byte != m_server.arg(key).toInt()) {
                        needSave = true;
                        *entry.value.byte = m_server.arg(key).toInt();
                    }
                    break;

                case HSDConfig::DataType::Word:
                    if (*entry.value.word != m_server.arg(key).toInt()) {
                        needSave = true;
                        *entry.value.word = m_server.arg(key).toInt();
                    }
                    break;

                case HSDConfig::DataType::Bool:
                    if (*entry.value.boolean != m_server.hasArg(key)) {
                        needSave = true;
                        *entry.value.boolean = m_server.hasArg(key);
                    }
                    break;
            }
        }
    }
    return needSave;
}

// ---------------------------------------------------------------------------------------------------------------------

void HSDWebserver::sendHeader(const char* title, bool useTable) {
    String header;
    header.reserve(useTable ? 750 : 500);

    header  = F("<!doctype html>\n<html lang=\"en\">\n"
                "<head>\n\t<meta charset='utf-8'>\n"
                "\t<title>");
    header += m_config.getHost();
    header += F("</title>\n"
                "\t<link rel='stylesheet' href='/css/layout.css'>\n"
                "\t<script type='text/javascript' src='/js/statusdisplay.js'></script>\n");
    if (useTable)
        header += F("\t<link rel='stylesheet' href='/css/tabulator.min.css'>\n"
                    "\t<script type='text/javascript' src='js/tabulator_core.min.js'></script>\n"
                    "\t<script type='text/javascript' src='js/accessor.min.js'></script>\n"
                    "\t<script type='text/javascript' src='js/ajax.min.js'></script>\n"
                    "\t<script type='text/javascript' src='js/edit.min.js'></script>\n"
                    "\t<script type='text/javascript' src='js/format.min.js'></script>\n"
                    "\t<script type='text/javascript' src='js/history.min.js'></script>\n"
                    "\t<script type='text/javascript' src='js/validate.min.js'></script>\n");     
    header += F("</head>\n"
                "<body>\n"
                "\t<span class='title'>");
    header += m_config.getHost();
    header += F("</span>V");
    header += HSD_VERSION;
    header += F("\n\t<form>\n\t\t<p>\n\t\t\t<input type='button' class='button' onclick=\"location.href='./'\" value='Status'>\n"
                "\t\t\t<input type='button' class='button' onclick='reboot()' value='Reboot'>\n"
                "\t\t\t<input type='button' class='button' onclick=\"location.href='./update'\" value='Update Firmware'>\n\t\t</p>\n"
                "\t\t<p>\n\t\t\t<input type='button' class='button' onclick=\"location.href='./cfgmain'\" value='Configuration'>\n"
                "\t\t\t<input type='button' class='button' onclick=\"location.href='./cfgcolormapping'\" value='Color mapping'>\n"
                "\t\t\t<input type='button' class='button' onclick=\"location.href='./cfgdevicemapping'\" value='Device mapping'>\n\t\t</p>\n\t</form>\n"
                "\t<h4>");
    header += title;
    header += F("</h4>\n");
    m_server.sendContent(header);
}

// ---------------------------------------------------------------------------------------------------------------------

String HSDWebserver::behavior2String(HSDConfig::Behavior behavior) const {
    String behaviorString = F("Off");
    switch(behavior) {
        case HSDConfig::Behavior::On:         behaviorString = F("On"); break;
        case HSDConfig::Behavior::Blinking:   behaviorString = F("Blink"); break;
        case HSDConfig::Behavior::Flashing:   behaviorString = F("Flash"); break;
        case HSDConfig::Behavior::Flickering: behaviorString = F("Flicker"); break;
        default: break;
    }
    return behaviorString;
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
        String contentType = getContentType(path);
//        m_server.sendHeader("Cache-Control", "max-age=86400");
        File file = SPIFFS.open(filepath, "r");
        size_t sent = m_server.streamFile(file, contentType);
        file.close();
        return true;
    }
    return false;    
}

// ---------------------------------------------------------------------------------------------------------------------

String HSDWebserver::getContentType(String filename) {
    if (filename.endsWith(".html")) {
        return "text/html";
    } else if (filename.endsWith(".css")) {
        return "text/css";
    } else if (filename.endsWith(".js")) {
        return "text/javascript"; // application
    } else if (filename.endsWith(".ico")) {
        return "image/x-icon";
    } else if (filename.endsWith(".json")) {
        m_server.sendHeader("Content-Disposition", "attachment; filename=" + (filename.lastIndexOf('/') == -1 ? filename : filename.substring(filename.lastIndexOf('/') + 1)));
       return "application/octet-stream;charset=utf-8";
    } else {
        return "text/plain";
    }
}

// ---------------------------------------------------------------------------------------------------------------------

void HSDWebserver::setUpdaterError() {
    Update.printError(Serial);
    StreamString str;
    Update.printError(str);
    m_updaterError = str.c_str();
}
