#include "HSDWebserver.hpp"

#ifdef ARDUINO_ARCH_ESP32
#include <Update.h>
#endif // ARDUINO_ARCH_ESP32

HSDWebserver::HSDWebserver(HSDConfig& config, const HSDLeds& leds, const HSDMqtt& mqtt) :
    m_config(config),
    m_deviceUptimeMinutes(0),
    m_leds(leds),
    m_mqtt(mqtt),
    m_server(80)
{
#ifndef ARDUINO_ARCH_ESP32
    m_updateServer.setup(&m_server);
#endif
}

// ---------------------------------------------------------------------------------------------------------------------

void HSDWebserver::begin() {
    Serial.println(F("Starting WebServer."));
    m_server.on("/", std::bind(&HSDWebserver::deliverStatusPage, this));
    m_server.on("/layout.css", std::bind(&HSDWebserver::deliverCSS, this));
    m_server.on("/cfgmain", std::bind(&HSDWebserver::deliverConfigPage, this));
    m_server.on("/cfgcolormapping", std::bind(&HSDWebserver::deliverColorMappingPage, this));
    m_server.on("/cfgdevicemapping", std::bind(&HSDWebserver::deliverDeviceMappingPage, this));
    m_server.on("/cfgExport", HTTP_GET, [=]() {
        String content;
        if (m_config.readFile(FILENAME_MAINCONFIG, content)) {
            m_server.sendHeader("Content-Disposition", "attachment; filename=" + m_config.getHost() + "_export.json");
            m_server.send(200, "application/octet-stream;charset=utf-8", content);
        } else {
            deliverNotFoundPage();
        }
    });
    m_server.on("/cfgExportDev", HTTP_GET, [=]() {
        String content;
        if (m_config.readFile(FILENAME_DEVMAPPING, content)) {
            m_server.sendHeader("Content-Disposition", "attachment; filename=" + m_config.getHost() + "_devMapping.json");
            m_server.send(200, "application/octet-stream;charset=utf-8", content);
        } else {
            deliverNotFoundPage();
        }
    });
    m_server.on("/cfgExportCol", HTTP_GET, [=]() {
        String content;
        if (m_config.readFile(FILENAME_COLORMAPPING, content)) {
            m_server.sendHeader("Content-Disposition", "attachment; filename=" + m_config.getHost() + "_colMapping.json");
            m_server.send(200, "application/octet-stream;charset=utf-8", content);
        } else {
            deliverNotFoundPage();
        }
    });
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
            Serial.printf("Import config: %s\n", upload.filename.c_str());
            m_file = SPIFFS.open(FILENAME_MAINCONFIG, "w+");
            if (!m_file)
                Serial.printf("Failed to open %s\n", FILENAME_MAINCONFIG);
        } else if (upload.status == UPLOAD_FILE_WRITE) {
            if (m_file && m_file.write(upload.buf, upload.currentSize) != upload.currentSize)
                Serial.println("Failed to write file");
        } else if (upload.status == UPLOAD_FILE_END) {
            if (m_file) {
                m_file.close();
                m_config.readMainConfigFile();
            }
            Serial.setDebugOutput(false);
        } else {
            Serial.printf("Config Import Failed Unexpectedly (likely broken connection): status=%d\n", upload.status);
        }
    });
#ifdef ARDUINO_ARCH_ESP32
    m_server.on("/update", HTTP_GET, [=]() {
        m_server.setContentLength(CONTENT_LENGTH_UNKNOWN);
        m_server.send(200);
        sendHeader("Update");
        m_server.sendContent(F("\t<form method='POST' action='/update' enctype='multipart/form-data'>\n\t\t<input type='file' name='update'>\n\t\t<input type='submit' value='Update'>\n\t</form>\n</body>"));
        m_server.sendContent("");
    });
    m_server.on("/update", HTTP_POST, [=]() {
        m_server.sendHeader("Connection", "close");
        m_server.send(200, "text/plain", (Update.hasError()) ? "FAIL" : "OK");
        ESP.restart();
    }, [=]() {
        HTTPUpload& upload = m_server.upload();
        if (upload.status == UPLOAD_FILE_START) {
            Serial.setDebugOutput(true);
            Serial.printf("Update: %s\n", upload.filename.c_str());
            if (!Update.begin()) { //start with max available size
                Update.printError(Serial);
            }
        } else if (upload.status == UPLOAD_FILE_WRITE) {
            if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
                Update.printError(Serial);
            }
        } else if (upload.status == UPLOAD_FILE_END) {
            if (Update.end(true)) { //true to set the size to the current progress
                Serial.printf("Update Success: %u\nRebooting...\n", upload.totalSize);
            } else {
                Update.printError(Serial);
            }
            Serial.setDebugOutput(false);
        } else {
            Serial.printf("Update Failed Unexpectedly (likely broken connection): status=%d\n", upload.status);
        }
    });
#endif // ARDUINO_ARCH_ESP32
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

    const HSDConfig::ConfigEntry* entries = m_config.cfgEntries();
    HSDConfig::Group lastGroup = HSDConfig::Group::_Last;
    int idx(0);
    do {
        if (entries[idx].group != lastGroup) {
            lastGroup = entries[idx].group;
            m_server.sendContent(html);
            html  = F("\t\t\t<tr><td class=\"cfgTitle\" colspan=\"2\">");
            html += m_config.groupDescription(entries[idx].group);
            html += F("</td></tr>\n");
        }
        html += F("\t\t\t<tr><td>");
        html += entries[idx].label;
        html += F("</td><td><input name=\"");
        html += m_config.groupDescription(entries[idx].group);
        html += F(".");
        html += entries[idx].key;
        if (entries[idx].placeholder != nullptr) {
            html += F("\" placeholder=\"");
            html += entries[idx].placeholder;
        }
        html += F("\" type=\"");
        switch (entries[idx].type) {
            case HSDConfig::DataType::String:
            case HSDConfig::DataType::Password:
                html += entries[idx].type == HSDConfig::DataType::Password ? F("password") : F("text");
                html += F("\" size=\"30\" value=\"");
                html += *(reinterpret_cast<String*>(entries[idx].val));
                html += F("\">");
                break;

            case HSDConfig::DataType::Byte:
                html += F("text\" size=\"30\" maxlength=\"");
                html += String(entries[idx].maxLength);
                html += F("\" value=\"");
                html += String(*(reinterpret_cast<uint8_t*>(entries[idx].val)));
                html += F("\">");
                break;

            case HSDConfig::DataType::Word:
                html += F("text\" size=\"30\" maxlength=\"");
                html += String(entries[idx].maxLength);
                html += F("\" value=\"");
                html += String(*(reinterpret_cast<uint16_t*>(entries[idx].val)));
                html += F("\">");
                break;

            case HSDConfig::DataType::Bool:
                html += F("checkbox\"");
                if (*(reinterpret_cast<bool*>(entries[idx].val)))
                    html += F(" checked");
                html += F(">");
                break;
        }
        html += F("</td></tr>\n");
    } while (entries[++idx].group != HSDConfig::Group::_Last);

    html += F("\t\t</table>\n"
             "\t\t<p>\n"
             "\t\t\t<input type='submit' class='button' value='Save'>\n"
             "\t\t\t<input type='button' class='button' onclick=\"location.href='./cfgExport'\" value='Export'>\n"
             "\t\t\t<input type='button' class='button' onclick=\"location.href='./cfgImport'\" value='Import'>\n"
             "\t\t</p>\n"
             "\t</form>\n</body>\n</html>");
    m_server.sendContent(html);
    m_server.sendContent("");

    if (needSave) {
        Serial.println(F("Main config has changed, storing it."));
        m_config.saveMain();
    }

    checkReboot();
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
    html += F("</p>\n");
    html += F("<p>Device voltage: ");
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

    checkReboot();
}

// ---------------------------------------------------------------------------------------------------------------------

void HSDWebserver::deliverColorMappingPage() {
    if (needUndo()) {
        Serial.println(F("Need to undo changes to color mapping config"));
        m_config.updateColorMapping();
    } else if (needAdd()) {
        Serial.println(F("Need to add color mapping config entry"));
        addColorMappingEntry();
    } else if(needDelete()) {
        Serial.println(F("Need to delete color mapping config entry"));
        deleteColorMappingEntry();
    } else if(needDeleteAll()) {
        Serial.println(F("Need to delete all color mapping config entries"));
        m_config.deleteAllColorMappingEntries();
    } else if(needSave()) {
        Serial.println(F("Need to save color mapping config"));
        m_config.saveColorMapping();
    }

    String html;
    html.reserve(1000);

    m_server.setContentLength(CONTENT_LENGTH_UNKNOWN);
    m_server.send(200);
    sendHeader("Color mapping configuration");
    m_server.sendContent(F("\t<table class=\"colMap\">\n"
                           "\t\t<tr class=\"colMap\">"
                           "<td class=\"colMapTitle\">Nr</td>"
                           "<td class=\"colMapTitle\">Message</td>"
                           "<td class=\"colMapTitle\">Color</td>"
                           "<td class=\"colMapTitle\">Behavior</td>"
                           "</tr>\n"));

    for (uint32_t i = 0; i < m_config.getNumberOfColorMappingEntries(); i++) {
        const HSDConfig::ColorMapping* mapping = m_config.getColorMapping(i);
        sendColorMappingTableEntry(i, mapping, m_config.hex2string(mapping->color));
    }

    html  = F("\t</table>\n");
    html += F("\t<p>Default colors you can use instead of HEX colors:<br>\n\t");
    for (uint8_t i = 1; i < NUMBER_OF_DEFAULT_COLORS; i++) {
        String temp = m_config.DefaultColor[i].key;
        temp.toLowerCase();
        html += temp;
        html += F(" ");
    }
    html += F("</p>\n");
    m_server.sendContent(html);

    html = "";
    if (m_config.isColorMappingFull()) {
        html += F("\t<p>Edit entry (add not possible, entry limit reached):</p>\n");
        html += getColorMappingTableAddEntryForm(m_config.getNumberOfColorMappingEntries(), true);
    } else {
        html += F("\t<p>Add/edit entry:</p>\n");
        html += getColorMappingTableAddEntryForm(m_config.getNumberOfColorMappingEntries(), false);
    }
    html += F("\t<p>\n\t\tDelete Entry:</p>\n");
    html += getDeleteForm();
    if (m_config.isColorMappingDirty()) {
        html += F("\t<p style='color:red'>Unsaved changes! Press Save to make them permanent,<br/>\n\tor Undo to revert to last saved version!</p>\n");
        html += getSaveForm();
    }
    html += F("</body>\n</html>");
    m_server.sendContent(html);
    m_server.sendContent("");

    checkReboot();

    Serial.print(F("Free RAM: ")); Serial.println(ESP.getFreeHeap());
}

// ---------------------------------------------------------------------------------------------------------------------

bool HSDWebserver::addColorMappingEntry() {
    bool success(false);
    if (m_server.hasArg("i") && m_server.hasArg("n") && m_server.hasArg("c") && m_server.hasArg("b")) {
        if (m_server.arg("n") != "") {
            uint32_t color = m_config.getDefaultColor(m_server.arg("c"));
            if (color == 0)
                color = m_config.string2hex(m_server.arg("c"));
            success = m_config.addColorMappingEntry(m_server.arg("i").toInt(), m_server.arg("n"), color,
                                                   (HSDConfig::Behavior)(m_server.arg("b").toInt()));
        } else {
            Serial.print(F("Skipping empty entry"));
        }
    }
    return success;
}

// ---------------------------------------------------------------------------------------------------------------------

bool HSDWebserver::deleteColorMappingEntry() {
    bool success(false);
    int entryNum(0);

    if (m_server.hasArg("i")) {
        entryNum = m_server.arg("i").toInt();
// TODO check conversion status
        success = m_config.deleteColorMappingEntry(entryNum);
    }
    return success;
}

// ---------------------------------------------------------------------------------------------------------------------

void HSDWebserver::deliverDeviceMappingPage() {
    if (needUndo()) {
        Serial.println(F("Need to undo changes to device mapping config"));
        m_config.updateDeviceMapping();
    } else if(needAdd()) {
        Serial.println(F("Need to add device mapping config entry"));
        addDeviceMappingEntry();
    } else if(needDelete()) {
        Serial.println(F("Need to delete device mapping config entry"));
        deleteDeviceMappingEntry();
    } else if(needDeleteAll()) {
        Serial.println(F("Need to delete all device mapping config entries"));
        m_config.deleteAllDeviceMappingEntries();
    } else if(needSave()) {
        Serial.println(F("Need to save device mapping config"));
        m_config.saveDeviceMapping();
    }

    String html;
    html.reserve(1000);

    m_server.setContentLength(CONTENT_LENGTH_UNKNOWN);
    m_server.send(200);
    sendHeader("Device mapping configuration");
    m_server.sendContent(F("\t<table class=\"colMap\">\n"
                           "\t\t<tr class=\"colMap\">"
                           "<td class=\"colMap\">Nr</td>"
                           "<td class=\"colMap\">Device</td>"
                           "<td class=\"colMap\">Led</td>"
                           "</tr>\n"));
    for (uint32_t i = 0; i < m_config.getNumberOfDeviceMappingEntries(); i++) {
        const HSDConfig::DeviceMapping* mapping = m_config.getDeviceMapping(i);
        sendDeviceMappingTableEntry(i, mapping);
    }

    html = F("\t</table>\n");
    if (m_config.isDeviceMappingFull()) {
        html += F("\t<p>Edit entry (add not possible, entry limit reached):</p>\n");
        html += getDeviceMappingTableAddEntryForm(m_config.getNumberOfDeviceMappingEntries(), true);
    } else {
        html += F("\t<p>Add/edit entry:</p>\n");
        html += getDeviceMappingTableAddEntryForm(m_config.getNumberOfDeviceMappingEntries(), false);
    }
    html += F("\t<p>Delete Entry:</p>\n");
    html += getDeleteForm();
    if (m_config.isDeviceMappingDirty()) {
        html += F("\t<p style='color:red'>Unsaved changes! Press ""Save"" to make them permanent, or they will be lost on next reboot!</p>\n");
        html += getSaveForm();
    }
    html += F("</body>\n</html>");
    m_server.sendContent(html);
    m_server.sendContent("");

    checkReboot();

    Serial.print(F("Free RAM: ")); Serial.println(ESP.getFreeHeap());
}

// ---------------------------------------------------------------------------------------------------------------------

bool HSDWebserver::addDeviceMappingEntry() {
    bool success(false);
    if (m_server.hasArg("i") && m_server.hasArg("n") && m_server.hasArg("l")) {
        if (m_server.arg("n") != "")
            success = m_config.addDeviceMappingEntry(m_server.arg("i").toInt(), m_server.arg("n"), m_server.arg("l").toInt());
        else
            Serial.print(F("Skipping empty entry"));
    }
    return success;
}

// ---------------------------------------------------------------------------------------------------------------------

bool HSDWebserver::deleteDeviceMappingEntry() {
    bool success(false);
    int entryNum(0);

    if (m_server.hasArg("i")) {
        entryNum = m_server.arg("i").toInt();
// TODO check conversion status
        success = m_config.deleteDeviceMappingEntry(entryNum);
    }

    return success;
}

// ---------------------------------------------------------------------------------------------------------------------

void HSDWebserver::deliverNotFoundPage() {
    String html = F("File Not Found\n\n");
    html += F("URI: ");
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

// ---------------------------------------------------------------------------------------------------------------------

void HSDWebserver::checkReboot() {
    if (m_server.hasArg(F("reset"))) {
        Serial.println(F("Rebooting ESP."));
        ESP.restart();
    }
}

// ---------------------------------------------------------------------------------------------------------------------

bool HSDWebserver::updateMainConfig() {
    bool needSave(false);
    const HSDConfig::ConfigEntry* entries = m_config.cfgEntries();
    String key = m_config.groupDescription(entries[0].group) + "." + entries[0].key;
    if (m_server.hasArg(key)) {
        for (int idx = 0; entries[idx].group != HSDConfig::Group::_Last; idx++) {
            key = m_config.groupDescription(entries[idx].group) + "." + entries[idx].key;
            switch (entries[idx].type) {
                case HSDConfig::DataType::String:
                case HSDConfig::DataType::Password:
                    if (*(reinterpret_cast<String*>(entries[idx].val)) != m_server.arg(key)) {
                        needSave = true;
                        *(reinterpret_cast<String*>(entries[idx].val)) = m_server.arg(key);
                    }
                    break;

                case HSDConfig::DataType::Byte:
                    if (*(reinterpret_cast<uint8_t*>(entries[idx].val)) != m_server.arg(key).toInt()) {
                        needSave = true;
                        *(reinterpret_cast<uint8_t*>(entries[idx].val)) = m_server.arg(key).toInt();
                    }
                    break;

                case HSDConfig::DataType::Word:
                    if (*(reinterpret_cast<uint16_t*>(entries[idx].val)) != m_server.arg(key).toInt()) {
                        needSave = true;
                        *(reinterpret_cast<uint16_t*>(entries[idx].val)) = m_server.arg(key).toInt();
                    }
                    break;

                case HSDConfig::DataType::Bool:
                    if (*(reinterpret_cast<bool*>(entries[idx].val)) != m_server.hasArg(key)) {
                        needSave = true;
                        *(reinterpret_cast<bool*>(entries[idx].val)) = m_server.hasArg(key);
                    }
                    break;
            }
        }
    }
    return needSave;
}

// ---------------------------------------------------------------------------------------------------------------------

void HSDWebserver::deliverCSS() {
    Serial.println(F("Delivering /layout.css"));
    m_server.sendHeader("Cache-Control", "max-age=86400"); // load only once a day
    m_server.send(200, F("text/css"), F(
        "body { background-color:#e5e5e5; font-family:Verdana,Arial,Helvetica; }\n"
        "span.title { font-size: xx-large; }\n"
        "table.cfg { border-spacing:2; border-width:0; width:400px; }\n"
        "table.colMap { border-spacing:2; border-width:1; }\n"
        "td { padding:0; }\n"
        "td.cfgTitle { font-size: large; font-weight: bold; }\n"
        "td.colMap { padding:1; }\n"
        "td.colMapTitle { padding:1; font-size: large; font-weight: bold; }\n"
        "tr.colMap { background-color:#828282; }\n"
        ".button { border-radius:0; height:30px; width:120px; border:0; background-color:black; color:#fff; margin:5px; cursor:pointer; }\n" // Header buttons
        ".buttonr {border-radius:0; height:30px; width:120px; border:0; background-color:red; color:#fff; margin:5px; cursor:pointer; }\n"
        ".hsdcolor { width:15px; height:15px; border:1px black solid; float:left; margin-right:5px; }\n" // Used on status page
        ".rdark { background-color:#f9f9f9; }\n"
        ".rlight { background-color:#e5e5e5; }\n"
    ));
}

// ---------------------------------------------------------------------------------------------------------------------

void HSDWebserver::sendHeader(const char* title) {
    String header;
    header.reserve(500);

    header  = F("<!doctype html>\n<html lang=\"en\">\n");
    header += F("<head>\n\t<meta charset='utf-8'>\n");
    header += F("\t<title>");
    header += m_config.getHost();
    header += F("</title>\n");
    header += F("\t<link rel=\"stylesheet\" href=\"/layout.css\">\n");
    header += F("</head>\n");
    header += F("<body>\n");
    header += F("\t<span class=\"title\">");
    header += m_config.getHost();
    header += F("</span>V");
    header += HSD_VERSION;
    header += F("\n");
    header += F("\t<form>\n\t\t<p>\n\t\t\t<input type='button' class='button' onclick=\"location.href='./'\" value='Status'>\n");
    header += F("\t\t\t<input type='submit' class='button' value='Reboot' name='reset'>\n");
    header += F("\t\t\t<input type='button' class='button' onclick=\"location.href='./update'\" value='Update Firmware'>\n\t\t</p>\n");
    header += F("\t\t<p>\n\t\t\t<input type='button' class='button' onclick=\"location.href='./cfgmain'\" value='Configuration'>\n");
    header += F("\t\t\t<input type='button' class='button' onclick=\"location.href='./cfgcolormapping'\" value='Color mapping'>\n");
    header += F("\t\t\t<input type='button' class='button' onclick=\"location.href='./cfgdevicemapping'\" value='Device mapping'>\n\t\t</p>\n\t</form>\n");
    header += F("\t<h4>");
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

void HSDWebserver::sendColorMappingTableEntry(int entryNum, const HSDConfig::ColorMapping* mapping,
                                              const String& colorString) {
    String html;
    html = entryNum % 2 == 0 ? F("\t\t<tr class='rlight'><td>") : F("\t\t<tr class='rdark'><td>");
    html += entryNum;
    html += F("</td><td>");
    html += mapping->msg;
    html += F("</td><td>");
    html += F("<div class='hsdcolor' style='background-color:");
    html += colorString;
    html += F(";'></div>");
    html += colorString;
    html += F("</td><td>");
    html += behavior2String(mapping->behavior);
    html += F("</td></tr>\n");
    m_server.sendContent(html);
}

// ---------------------------------------------------------------------------------------------------------------------

void HSDWebserver::sendDeviceMappingTableEntry(int entryNum, const HSDConfig::DeviceMapping* mapping) {
    String html = entryNum % 2 == 0 ? F("\t\t<tr class='rlight'><td>") : F("\t\t<tr class='rdark'><td>");
    html += entryNum;
    html += F("</td><td>");
    html += mapping->name;
    html += F("</td><td>");
    html += mapping->ledNumber;
    html += F("</td></tr>\n");
    m_server.sendContent(html);
}

// ---------------------------------------------------------------------------------------------------------------------

String HSDWebserver::getDeviceMappingTableAddEntryForm(int newEntryNum, bool isFull) const {
    String html = F("\t<form>\n\t\t<table>\n\t\t\t<tr>\n");
    html += F("\t\t\t\t<td><input type='text' name='i' value='");
    html += isFull ? newEntryNum - 1 : newEntryNum;
    html += F("' size='5' maxlength='3' placeholder='Nr'></td>\n");
    html += F("\t\t\t\t<td><input type='text' name='n' value='' size='30' maxlength='25' placeholder='device name'></td>\n");
    html += F("\t\t\t\t<td><input type='text' name='l' value='");
    html += isFull ? newEntryNum - 1 : newEntryNum;
    html += F("' size='6' maxlength='3' placeholder='led nr'></td>\n\t\t\t</tr>\n\t\t</table>\n");
    html += F("\t\t<input type='submit' class='button' value='");
    html += isFull ? F("Edit") : F("Add/Edit");
    html += F("' name='add'>\n\t</form>\n");
    return html;
}

// ---------------------------------------------------------------------------------------------------------------------

String HSDWebserver::getSaveForm() const {
    String html;
    html += F("\t<form>\n\t\t<input type='submit' class='buttonr' value='Save' name='save'>\n");
    html += F("\t\t<input type='submit' class='buttonr' value='Undo' name='undo'>\n\t</form>\n");
    return html;
}

// ---------------------------------------------------------------------------------------------------------------------

String HSDWebserver::getDeleteForm() const {
    String html;
    html += F("\t<form>\n\t\t<input type='text' name='i' value='' size='5' maxlength='3' placeholder='Nr'><br/>\n");
    html += F("\t\t<input type='submit' class='button' value='Delete' name='delete'>\n");
    html += F("\t\t<input type='submit' class='button' value='Delete all' name='deleteall'>\n\t</form>\n");
    return html;
}

// ---------------------------------------------------------------------------------------------------------------------

String HSDWebserver::getColorMappingTableAddEntryForm(int newEntryNum, bool isFull) const {
    String html;
    html += F("\t<form>\n\t\t<table>\n\t\t\t<tr>\n");
    html += F("\t\t\t\t<td><input type='text' name='i' value='");
    html += isFull ? newEntryNum - 1 : newEntryNum;
    html += F("' size='5' maxlength='3' placeholder='Nr'></td>\n");
    html += F("\t\t\t\t<td><input type='text' name='n' value='' size='20' maxlength='15' placeholder='message name'></td>\n");
    html += F("\t\t\t\t<td><input type='text' name='c' value='' size='10' maxlength='7' placeholder='#ffaabb' list='colorOptions'>");
    html += F("<datalist id='colorOptions'>\n");
    for (uint8_t i = 1; i < NUMBER_OF_DEFAULT_COLORS; i++) {
        String temp = HSDConfig::DefaultColor[i].key;
        temp.toLowerCase();
        html += F("\t\t\t\t\t<option value='");
        html += temp;
        html += F("'>");
        html += temp;
        html += F("</option>\n");
    }
    html += F("\t\t\t\t</datalist></td>\n");
    html += F("\t\t\t\t<td><select name='b'>");
    html += getBehaviorOptions(HSDConfig::Behavior::On);
    html += F("</select></td>\n\t\t\t</tr>\n\t\t</table>\n");
    html += F("\t\t<input type='submit' class='button' value='");
    html += isFull ? F("Edit") : F("Add/Edit");
    html += F("' name='add'>\n\t</form>\n");
    return html;
}

// ---------------------------------------------------------------------------------------------------------------------

#define SELECTED_STRING (F("selected='selected'"))
#define EMPTY_STRING    (F(""))

// ---------------------------------------------------------------------------------------------------------------------

String HSDWebserver::getBehaviorOptions(HSDConfig::Behavior selectedBehavior) const {
    String onSelect       =   (selectedBehavior == HSDConfig::Behavior::On)         ? SELECTED_STRING : EMPTY_STRING;
    String blinkingSelect =   (selectedBehavior == HSDConfig::Behavior::Blinking)   ? SELECTED_STRING : EMPTY_STRING;
    String flashingSelect =   (selectedBehavior == HSDConfig::Behavior::Flashing)   ? SELECTED_STRING : EMPTY_STRING;
    String flickeringSelect = (selectedBehavior == HSDConfig::Behavior::Flickering) ? SELECTED_STRING : EMPTY_STRING;

    String html;
    html += F("<option "); html += onSelect;         html += F(" value='"); html += static_cast<uint8_t>(HSDConfig::Behavior::On);         html += F("'>On</option>");
    html += F("<option "); html += blinkingSelect;   html += F(" value='"); html += static_cast<uint8_t>(HSDConfig::Behavior::Blinking);   html += F("'>Blink</option>");
    html += F("<option "); html += flashingSelect;   html += F(" value='"); html += static_cast<uint8_t>(HSDConfig::Behavior::Flashing);   html += F("'>Flash</option>");
    html += F("<option "); html += flickeringSelect; html += F(" value='"); html += static_cast<uint8_t>(HSDConfig::Behavior::Flickering); html += F("'>Flicker</option>");
    return html;
}
