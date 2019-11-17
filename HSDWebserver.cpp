#include "HSDWebserver.hpp"

#ifdef ARDUINO_ARCH_ESP32    
#include <Update.h>
#endif // ARDUINO_ARCH_ESP32

HSDWebserver::HSDWebserver(HSDConfig& config, const HSDLeds& leds, const HSDMqtt& mqtt) :
    m_config(config),
    m_deviceUptimeMinutes(0),
#ifdef HSD_SENSOR_ENABLED
    m_lastHum(0),
    m_lastTemp(0),
#endif // HSD_SENSOR_ENABLED
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
    m_server.on("/cfgmain", std::bind(&HSDWebserver::deliverConfigPage, this));
    m_server.on("/cfgcolormapping", std::bind(&HSDWebserver::deliverColorMappingPage, this));
    m_server.on("/cfgdevicemapping", std::bind(&HSDWebserver::deliverDeviceMappingPage, this));
#ifdef ARDUINO_ARCH_ESP32
    m_server.on("/update", HTTP_GET, [=]() {
        m_server.setContentLength(CONTENT_LENGTH_UNKNOWN);
        m_server.send(200);
        sendHeader("Update", m_config.getHost(), m_config.getVersion());
        m_server.sendContent(F("<form method='POST' action='/update' enctype='multipart/form-data'><input type='file' name='update'><input type='submit' value='Update'></form>"));
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
    sendHeader("General configuration", m_config.getHost(), m_config.getVersion());

    html = F("<form><font face='Verdana,Arial,Helvetica'>"
             "<table width='400px' border='0' cellpadding='0' cellspacing='2'>"
             " <tr>"
             "  <td><b><font size='+1'>General</font></b></td>"
             "  <td></td>"
             " </tr>"
             " <tr>"
             "  <td>Name</td>"
             "  <td><input type='text' name='host' value='");
    html += m_config.getHost();
    html += F("' size='30' maxlength='40' placeholder='host'></td></tr>");
    m_server.sendContent(html);
    
    html = F(" <tr>"
             "  <td><b><font size='+1'>WiFi</font></b></td>"
             "  <td></td>"
             " </tr>"
             " <tr>"
             "  <td>SSID</td>");
    html += F("<td><input type='text' name='wifiSSID' value='");
    html += m_config.getWifiSSID();
    html += F("' size='30' maxlength='40' placeholder='SSID'></td>");
    html += F("</tr><tr><td>Password</td>");
    html += F("  <td><input type='password' name='wifiPSK' value='");
    html += m_config.getWifiPSK();
    html += F("' size='30' maxlength='40' placeholder='Password'></td></tr>");
    m_server.sendContent(html);

    html = F(" <tr>"
             "  <td><b><font size='+1'>MQTT</font></b></td>"
             "  <td></td>"
             " </tr>"
             " <tr>"
             "  <td>Server</td>"
             "  <td><input type='text' name='mqttServer' value='");
    html += m_config.getMqttServer();
    html += F("' size='30' maxlength='40' placeholder='IP or hostname'></td></tr><tr><td>Port</td>");
    html += F("  <td><input type='text' name='mqttPort' value='");
    html += String(m_config.getMqttPort());
    html += F("' size='30' maxlength='5' placeholder='Port'></td></tr><tr><td>User</td>");
    html += F("  <td><input type='text' name='mqttUser' value='");
    html += String(m_config.getMqttUser());
    html += F("' size='30' maxlength='20' placeholder='User name'></td></tr><tr><td>Password</td>");
    html += F("  <td><input type='password' name='mqttPassword' value='");
    html += m_config.getMqttPassword();
    html += F("' size='30' maxlength='50' placeholder='Password'></td></tr><tr><td>Status topic</td>");  
    html += F("  <td><input type='text' name='mqttStatusTopic' value='");
    html += m_config.getMqttStatusTopic();
#ifdef MQTT_TEST_TOPIC  
    html += F("' size='30' maxlength='40' placeholder='#'></td>"
              " </tr>"
              " <tr>"
              "  <td>Test topic</td>"
              "  <td><input type='text' name='mqttTestTopic' value='");
    html += m_config.getMqttTestTopic();
#endif // MQTT_TEST_TOPIC  
    html += F("' size='30' maxlength='40' placeholder='#'></td>"
              " </tr>"
              " <tr>"
              "  <td>Outgoing topic</td>"
              "  <td><input type='text' name='mqttOutTopic' value='");
    html += m_config.getMqttOutTopic();
    html += F("' size='30' maxlength='40' placeholder='#'></td></tr>");
    m_server.sendContent(html);

    html = F(" <tr>"
             "  <td><b><font size='+1'>LEDs</font></b></td>"
             "  <td></td>"
             " </tr>"
             " <tr>"
             "  <td>Number of LEDs</td>");
    html += "  <td><input type='text' name='ledCount' value='" + String(m_config.getNumberOfLeds()) + "' size='30' maxlength='40' placeholder='0'></td></tr>";
    html += F("<tr><td>LED pin</td>");
    html += F("<td><input type='text' name='ledPin' value='");
    html += String(m_config.getLedDataPin());
    html += F("' size='30' maxlength='40' placeholder='0'></td></tr>");
    html += F("<tr><td>Brightness</td>");
    html += F("<td><input type='text' name='ledBrightness' value='");
    html += String(m_config.getLedBrightness());
    html += F("' size='30' maxlength='5' placeholder='0-255'></td></tr>"); 
    m_server.sendContent(html);
  
#ifdef HSD_CLOCK_ENABLED
    html = F(" <tr>"
             "  <td><b><font size='+1'>Clock</font></b></td>"
             "  <td>(TM1637)</td>"
             " </tr>"
             " <tr>"
             "  <td>Enabled</td>");
    html += F("  <td><input type='checkbox' name='clockEnabled'");
    if (m_config.getClockEnabled())
        html += F(" checked");
    html += F("></td></tr>");
    html += F("<tr><td>CLK pin</td>");
    html += "  <td><input type='text' name='clockCLK' value='" + String(m_config.getClockPinCLK()) + "' size='30' maxlength='5' placeholder='0'></td></tr>";
    html += F("<tr><td>DIO pin</td>");
    html += F("<td><input type='text' name='clockDIO' value='");
    html += String(m_config.getClockPinDIO());
    html += F("' size='30' maxlength='5' placeholder='0'></td></tr>");
    html += F("<tr><td>Brightness</td>");
    html += F("<td><input type='text' name='clockBrightness' value='");
    html += String(m_config.getClockBrightness());
    html += F("' size='30' maxlength='5' placeholder='0-8'></td></tr>"); 
    html += F("<tr><td>Time zone</td>");
    html += F("<td><input type='text' name='clockTZ' value='");
    html += m_config.getClockTimeZone();
    html += F("' size='30' maxlength='40' placeholder='CET-1CEST,M3.5.0/2,M10.5.0/3'></td></tr>"); 
    html += F("<tr><td>NTP server</td>");
    html += F("<td><input type='text' name='clockServer' value='");
    html += m_config.getClockNTPServer();
    html += F("' size='30' maxlength='40' placeholder='pool.ntp.org'></td></tr>");
    html += F("<tr><td>NTP update interval (min.)</td>");
    html += F("<td><input type='text' name='clockInterval' value='");
    html += String(m_config.getClockNTPInterval());
    html += F("' size='30' maxlength='5' placeholder='20'></td></tr>");
    m_server.sendContent(html);
#endif // HSD_
  
#ifdef HSD_SENSOR_ENABLED
    html = F(" <tr>"
             "  <td><b><font size='+1'>Sensor</font></b></td>"
             "  <td>(Sonoff SI7021)</td>"
             " </tr>"
             " <tr>"
             "  <td>Enabled</td>");
    html += F("  <td><input type='checkbox' name='sensorEnabled'");
    if (m_config.getSensorEnabled())
        html += F(" checked");
    html += F("></td></tr>");
    html += F("<tr><td>Data pin</td>");
    html += F("<td><input type='text' name='sensorPin' value='");
    html += String(m_config.getSensorPin());
    html += F("' size='30' maxlength='5' placeholder='0'></td></tr>");
    html += F("<tr><td>Sensor update interval (min.)</td>");
    html += F("<td><input type='text' name='sensorInterval' value='");
    html += String(m_config.getSensorInterval());
    html += F("' size='30' maxlength='5' placeholder='5'></td></tr>");  
    m_server.sendContent(html);
#endif

    html = F("</table>"
             "<input type='submit' class='button' value='Save'>"
             "</form></font></body></html>");
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
    sendHeader("Status", m_config.getHost(), m_config.getVersion());

    html = F("<p>Device uptime: ");
    char buffer[50];
    memset(buffer, 0, sizeof(buffer));
    sprintf(buffer, "%lu days, %lu hours, %lu minutes", m_deviceUptimeMinutes / 1440, (m_deviceUptimeMinutes / 60) % 24, 
            m_deviceUptimeMinutes % 60);
    html += String(buffer);
    html += F("</p>");

#ifdef ARDUINO_ARCH_ESP32
    html += F("<p>Device Heap stats (size, free, minFree, max) [Bytes]: ");
    html += String(ESP.getHeapSize()) + ", " + String(ESP.getFreeHeap()) + ", " + String(ESP.getMinFreeHeap()) + ", " + String(ESP.getMaxAllocHeap()) + "</p>";
#else    
    uint32_t free;
    uint16_t max;
    uint8_t frag;
    ESP.getHeapStats(&free, &max, &frag);

    html += F("<p>Device RAM stats (free, max, frag) [Bytes]: ");
    html += String(free) + ", " + String(max) + ", " + String(frag);
    html += F("</p>");
    html += F("<p>Device voltage: ");
    html += String(ESP.getVcc());
    html += F(" mV</p>");
#endif

#ifdef HSD_SENSOR_ENABLED
    if (m_config.getSensorEnabled()) {
        html += F("<p>Sensor: Temp ");
        html += String(m_lastTemp, 1);
        html += F("&deg;C, Hum ");
        html += String(m_lastHum, 1);
        html += F("%</p>");
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
        html += F("</b><br/><p>");
    } else {
        html += F("<p>Device is not connected to local network<p>");
    }

    if (m_mqtt.connected()) {
        html += F("<p>Device is connected to  MQTT broker <b>");
        html += m_config.getMqttServer();
        html += F("</b><p>");
    } else {
        html += F("<p>Device is not connected to an MQTT broker<p>");
    }
    m_server.sendContent(html);

    if (m_config.getNumberOfLeds() == 0) {
        html = F("<p>No LEDs configured yet<p>");
    } else {
        int ledOnCount(0);    
        html = F("<p>");
        for (int ledNr = 0; ledNr < m_config.getNumberOfLeds(); ledNr++) {
            uint32_t color = m_leds.getColor(ledNr);
            HSDConfig::Behavior behavior = m_leds.getBehavior(ledNr);
            String device = m_config.getDevice(ledNr);
            if ((m_config.getDefaultColor("NONE") != color) && (HSDConfig::OFF != behavior)) {
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
                html += F("</b><br/></p>");

                ledOnCount++;
            }
        }

        if (ledOnCount == 0)
            html += F("<p>All LEDs are <b>off</b><p>");

        html += F("</p>");
    }
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
    sendHeader("Color mapping configuration", m_config.getHost(), m_config.getVersion());
    m_server.sendContent(F("<table border='1' cellpadding='1' cellspacing='2'>"
                           " <tr style='background-color:#828282'>"
                           "  <td><b><font size='+1'>Nr</font></b></td>"
                           "  <td><b><font size='+1'>Message</font></b></td>"
                           "  <td><b><font size='+1'>Color</font></b></td>"
                           "  <td><b><font size='+1'>Behavior</font></b></td>"
                           " </tr>"));

    for (uint32_t i = 0; i < m_config.getNumberOfColorMappingEntries(); i++) {
        const HSDConfig::ColorMapping* mapping = m_config.getColorMapping(i);
        sendColorMappingTableEntry(i, mapping, m_config.hex2string(mapping->color));
    }

    html = F("</table>");
    html += F("<p>Default colors you can use instead of HEX colors:<br>");
    for (uint8_t i = 1; i < NUMBER_OF_DEFAULT_COLORS; i++) {
        String temp = m_config.DefaultColor[i].key;
        temp.toLowerCase();
        html += temp;
        html += F(" ");
    }
    html += F("</p>");
    m_server.sendContent(html);

    html = "";
    if (m_config.isColorMappingFull()) {
        html += F("</table><p>Edit entry (add not possible, entry limit reached):</p>");
        html += getColorMappingTableAddEntryForm(m_config.getNumberOfColorMappingEntries(), true);
    } else {
        html += F("</table><p>Add/edit entry:</p>");    
        html += getColorMappingTableAddEntryForm(m_config.getNumberOfColorMappingEntries(), false);
    }
    html += F("<p>Delete Entry:</p>");
    html += getDeleteForm();
    if (m_config.isColorMappingDirty()) {
        html += F("<p style='color:red'>Unsaved changes! Press Save to make them permanent, <br/>or Undo to revert to last saved version!</p>");
        html += getSaveForm();
    }
    html += getFooter();
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
    sendHeader("Device mapping configuration", m_config.getHost(), m_config.getVersion());
    m_server.sendContent(F("<table border='1' cellpadding='1' cellspacing='2'>"
                           " <tr style='background-color:#828282'>"
                           "  <td><b><font size='+1'>Nr</font></b></td>"  
                           "  <td><b><font size='+1'>Device</font></b></td>"
                           "  <td><b><font size='+1'>Led</font></b></td>"
                           " </tr>"));
    for (uint32_t i = 0; i < m_config.getNumberOfDeviceMappingEntries(); i++) {
        const HSDConfig::DeviceMapping* mapping = m_config.getDeviceMapping(i);
        sendDeviceMappingTableEntry(i, mapping);
    }

    html = F("</table>");
    if (m_config.isDeviceMappingFull()) {
        html += F("</table><p>Edit entry (add not possible, entry limit reached):</p>");
        html += getDeviceMappingTableAddEntryForm(m_config.getNumberOfDeviceMappingEntries(), true);
    } else {
        html += F("</table><p>Add/edit entry:</p>");    
        html += getDeviceMappingTableAddEntryForm(m_config.getNumberOfDeviceMappingEntries(), false);
    }
    html += F("<br/>Delete Entry:<br/>");
    html += getDeleteForm();
    if (m_config.isDeviceMappingDirty()) {
        html += F("<p style='color:red'>Unsaved changes! Press ""Save"" to make them permanent, or they will be lost on next reboot!</p>");
        html += getSaveForm();
    }
    html += getFooter();
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
    if (m_server.hasArg(JSON_KEY_HOST))
        needSave |= m_config.setHost(m_server.arg(JSON_KEY_HOST));
    if (m_server.hasArg(JSON_KEY_WIFI_SSID))
        needSave |= m_config.setWifiSSID(m_server.arg(JSON_KEY_WIFI_SSID));
    if (m_server.hasArg(JSON_KEY_WIFI_PSK)) 
        needSave |= m_config.setWifiPSK(m_server.arg(JSON_KEY_WIFI_PSK));
    if (m_server.hasArg(JSON_KEY_MQTT_SERVER))
        needSave |= m_config.setMqttServer(m_server.arg(JSON_KEY_MQTT_SERVER));
    if (m_server.hasArg(JSON_KEY_MQTT_USER))
        needSave |= m_config.setMqttUser(m_server.arg(JSON_KEY_MQTT_USER));
    if (m_server.hasArg(JSON_KEY_MQTT_PASSWORD))
        needSave |= m_config.setMqttPassword(m_server.arg(JSON_KEY_MQTT_PASSWORD));
    if (m_server.hasArg(JSON_KEY_MQTT_PORT))
        needSave |= m_config.setMqttPort(m_server.arg(JSON_KEY_MQTT_PORT).toInt());
    if (m_server.hasArg(JSON_KEY_MQTT_STATUS_TOPIC))
        needSave |= m_config.setMqttStatusTopic(m_server.arg(JSON_KEY_MQTT_STATUS_TOPIC));
#ifdef MQTT_TEST_TOPIC  
    if (m_server.hasArg(JSON_KEY_MQTT_TEST_TOPIC)) 
        needSave |= m_config.setMqttTestTopic(m_server.arg(JSON_KEY_MQTT_TEST_TOPIC));
#endif // MQTT_TEST_TOPIC
    if (m_server.hasArg(JSON_KEY_MQTT_OUT_TOPIC)) 
        needSave |= m_config.setMqttOutTopic(m_server.arg(JSON_KEY_MQTT_OUT_TOPIC));

    if (m_server.hasArg(JSON_KEY_LED_COUNT)) {
        int ledCount = m_server.arg(JSON_KEY_LED_COUNT).toInt();
        if (ledCount > 0)
            needSave |= m_config.setNumberOfLeds(ledCount);
    }
  
    if (m_server.hasArg(JSON_KEY_LED_PIN)) {
        int ledPin = m_server.arg(JSON_KEY_LED_PIN).toInt();
        if (ledPin > 0)
            needSave |= m_config.setLedDataPin(ledPin);
    }

    if (m_server.hasArg(JSON_KEY_LED_BRIGHTNESS)) {
        uint8_t ledBrightness = m_server.arg(JSON_KEY_LED_BRIGHTNESS).toInt();
        if (ledBrightness > 0)
            needSave |= m_config.setLedBrightness(ledBrightness);
    }
#ifdef HSD_CLOCK_ENABLED
    if (m_server.hasArg(JSON_KEY_CLOCK_PIN_CLK)) {
        needSave |= m_config.setClockEnabled(m_server.hasArg(JSON_KEY_CLOCK_ENABLED));
        int pin = m_server.arg(JSON_KEY_CLOCK_PIN_CLK).toInt();
        if (pin > 0)
            needSave |= m_config.setClockPinCLK(pin);
    }

    if (m_server.hasArg(JSON_KEY_CLOCK_PIN_DIO)) {
        int pin = m_server.arg(JSON_KEY_CLOCK_PIN_DIO).toInt();    
        if (pin > 0)
            needSave |= m_config.setClockPinDIO(pin);
    }

    if (m_server.hasArg(JSON_KEY_CLOCK_BRIGHTNESS)) {
        int brightness = m_server.arg(JSON_KEY_CLOCK_BRIGHTNESS).toInt();
        if (brightness >= 0 && brightness < 9)
            needSave |= m_config.setClockBrightness(brightness);
    }

    if (m_server.hasArg(JSON_KEY_CLOCK_TIME_ZONE)) 
        needSave |= m_config.setClockTimeZone(m_server.arg(JSON_KEY_CLOCK_TIME_ZONE));
    if (m_server.hasArg(JSON_KEY_CLOCK_NTP_SERVER)) 
        needSave |= m_config.setClockNTPServer(m_server.arg(JSON_KEY_CLOCK_NTP_SERVER));

    if (m_server.hasArg(JSON_KEY_CLOCK_NTP_INTERVAL)) {
        int interval = m_server.arg(JSON_KEY_CLOCK_NTP_INTERVAL).toInt();
        if (interval > 0)
            needSave |= m_config.setClockNTPInterval(interval);
    }
#endif // HSD_CLOCK_ENABLED
#ifdef HSD_SENSOR_ENABLED
    if (m_server.hasArg(JSON_KEY_SENSOR_PIN)) {
        needSave |= m_config.setSensorEnabled(m_server.hasArg(JSON_KEY_SENSOR_ENABLED));
        needSave |= m_config.setSensorPin(m_server.arg(JSON_KEY_SENSOR_PIN).toInt());
    }
  
    if (m_server.hasArg(JSON_KEY_SENSOR_INTERVAL))
        needSave |= m_config.setSensorInterval(m_server.arg(JSON_KEY_SENSOR_INTERVAL).toInt());
#endif // HSD_SENSOR_ENABLED  
  
    return needSave;
}

// ---------------------------------------------------------------------------------------------------------------------

void HSDWebserver::sendHeader(const char* title, const String& host, const String& version) {
    String header;
    header.reserve(1500);

    header  = F("<!doctype html> <html>");
    header += F("<head><meta charset='utf-8'>");
    header += F("<title>");
    header += host;
    header += F("</title>");
    header += F("<style>.button {border-radius:0;height:30px;width:120px;border:0;background-color:black;color:#fff;margin:5px;cursor:pointer;}</style>");
    header += F("<style>.buttonr {border-radius:0;height:30px;width:120px;border:0;background-color:red;color:#fff;margin:5px;cursor:pointer;}</style>");
    header += F("<style>.hsdcolor {width:15px;height:15px;border:1px black solid;float:left;margin-right:5px';}</style>");
    header += F("<style>.rdark {background-color:#f9f9f9;}</style>");
    header += F("<style>.rlight {background-color:#e5e5e5;}</style>");
    header += F("</head>");
    header += F("<body bgcolor='#e5e5e5'><font face='Verdana,Arial,Helvetica'>");
    header += F("<font size='+3'>");
    header += host;
    header += F("</font><font size='-3'>V");
    header += version;
    header += F("</font>");
    header += F("<form><p><input type='button' class='button' onclick=\"location.href='./'\" value='Status'>");
    header += F("<input type='submit' class='button' value='Reboot' name='reset'>");
    header += F("<input type='button' class='button' onclick=\"location.href='./update'\" value='Update Firmware'></p>");
  
    header += F("<p><input type='button' class='button' onclick=\"location.href='./cfgmain'\" value='Configuration'>");
    header += F("<input type='button' class='button' onclick=\"location.href='./cfgcolormapping'\" value='Color mapping'>");
    header += F("<input type='button' class='button' onclick=\"location.href='./cfgdevicemapping'\" value='Device mapping'></p></form>");
  
    header += F("<h4>"); 
    header += title;
    header += F("</h4>");

    Serial.print(F("Header size: ")); Serial.println(header.length());
  
    m_server.sendContent(header);
}

// ---------------------------------------------------------------------------------------------------------------------

String HSDWebserver::behavior2String(HSDConfig::Behavior behavior) const {
    String behaviorString = F("Off");  
    switch(behavior) {
        case HSDConfig::ON:         behaviorString = F("On"); break;
        case HSDConfig::BLINKING:   behaviorString = F("Blink"); break;
        case HSDConfig::FLASHING:   behaviorString = F("Flash"); break;
        case HSDConfig::FLICKERING: behaviorString = F("Flicker"); break;
        default: break;
    }
    return behaviorString;
}

// ---------------------------------------------------------------------------------------------------------------------

void HSDWebserver::sendColorMappingTableEntry(int entryNum, const HSDConfig::ColorMapping* mapping, 
                                              const String& colorString) {
    String html;
    html = entryNum % 2 == 0 ? F("<tr class='rlight'><td>") : F("<tr class='rdark'><td>");
    html += entryNum;
    html += F("</td><td>");
    html += mapping->msg;
    html += F("</td><td>");
    html += F("<div class='hsdcolor' style='background-color:");
    html += colorString;
    html += F("';></div> ");
    html += colorString;
    html += F("</td><td>"); 
    html += behavior2String(mapping->behavior);
    html += F("</td></tr>");
        
    m_server.sendContent(html);
}

// ---------------------------------------------------------------------------------------------------------------------

void HSDWebserver::sendDeviceMappingTableEntry(int entryNum, const HSDConfig::DeviceMapping* mapping) {
    String html = entryNum % 2 == 0 ? F("<tr class='rlight'><td>") : F("<tr class='rdark'><td>");
    html += entryNum;
    html += F("</td><td>");
    html += mapping->name;
    html += F("</td><td>");
    html += mapping->ledNumber;
    html += F("</td></tr>");

    m_server.sendContent(html);
}

// ---------------------------------------------------------------------------------------------------------------------

String HSDWebserver::getDeviceMappingTableAddEntryForm(int newEntryNum, bool isFull) const {
    String html = F("<form><table><tr>");
    html += F("<td><input type='text' name='i' value='");
    html += isFull ? newEntryNum - 1 : newEntryNum;
    html += F("' size='5' maxlength='3' placeholder='Nr'</td>");
    html += F("<td><input type='text' name='n' value='' size='30' maxlength='25' placeholder='device name'></td>");
    html += F("<td><input type='text' name='l' value='");
    html += isFull ? newEntryNum - 1 : newEntryNum;
    html += F("' size='6' maxlength='3' placeholder='led nr'></td></tr></table>");
    html += F("<input type='submit' class='button' value='");
    html += isFull ? F("Edit") : F("Add/Edit");
    html += F("' name='add'></form>");

    return html;
}

// ---------------------------------------------------------------------------------------------------------------------

String HSDWebserver::getSaveForm() const {
    String html;
    html += F("<form><input type='submit' class='buttonr' value='Save' name='save'>");   
    html += F("<form><input type='submit' class='buttonr' value='Undo' name='undo'></form>");   
    return html;
}

// ---------------------------------------------------------------------------------------------------------------------

String HSDWebserver::getDeleteForm() const {
    String html;  
    html += F("<form><input type='text' name='i' value='' size='5' maxlength='3' placeholder='Nr'><br/>");
    html += F("<input type='submit' class='button' value='Delete' name='delete'>");
    html += F("<input type='submit' class='button' value='Delete all' name='deleteall'></form>");
  
    return html;  
}

// ---------------------------------------------------------------------------------------------------------------------

String HSDWebserver::getColorMappingTableAddEntryForm(int newEntryNum, bool isFull) const {
    String html;  
    html += F("<form><table><tr>");
    html += F("<td><input type='text' name='i' value='");
    html += isFull ? newEntryNum - 1 : newEntryNum;
    html += F("' size='5' maxlength='3' placeholder='Nr'</td>");
    html += F("<td><input type='text' name='n' value='' size='20' maxlength='15' placeholder='message name'></td>");
    html += F("<td><input type='text' name='c' value='' size='10' maxlength='7' placeholder='#ffaabb' list='colorOptions'>");
    html += F("<datalist id='colorOptions'>");
    for (uint8_t i = 1; i < NUMBER_OF_DEFAULT_COLORS; i++) {
        String temp = HSDConfig::DefaultColor[i].key;
        temp.toLowerCase();
        html += F("<option value='");
        html += temp;
        html += F("'>");
        html += temp;
        html += F("</option>");
    }
    html += F("</datalist>");  
    html += F("</td>");
    html += F("<td><select name='b'>");
    html += getBehaviorOptions(HSDConfig::ON);
    html += F("</select></td></tr></table>");  
    html += F("<input type='submit' class='button' value='");
    html += isFull ? F("Edit") : F("Add/Edit");
    html += F("' name='add'></form>");

    return html;
}

// ---------------------------------------------------------------------------------------------------------------------

#define SELECTED_STRING (F("selected='selected'"))
#define EMPTY_STRING    (F(""))

// ---------------------------------------------------------------------------------------------------------------------

String HSDWebserver::getBehaviorOptions(HSDConfig::Behavior selectedBehavior) const {
    String onSelect       =   (selectedBehavior == HSDConfig::ON)         ? SELECTED_STRING : EMPTY_STRING;
    String blinkingSelect =   (selectedBehavior == HSDConfig::BLINKING)   ? SELECTED_STRING : EMPTY_STRING;
    String flashingSelect =   (selectedBehavior == HSDConfig::FLASHING)   ? SELECTED_STRING : EMPTY_STRING;
    String flickeringSelect = (selectedBehavior == HSDConfig::FLICKERING) ? SELECTED_STRING : EMPTY_STRING;
  
    String html;
    html += F("<option "); html += onSelect;         html += F(" value='"); html += HSDConfig::ON;         html += F("'>On</option>");
    html += F("<option "); html += blinkingSelect;   html += F(" value='"); html += HSDConfig::BLINKING;   html += F("'>Blink</option>");
    html += F("<option "); html += flashingSelect;   html += F(" value='"); html += HSDConfig::FLASHING;   html += F("'>Flash</option>");
    html += F("<option "); html += flickeringSelect; html += F(" value='"); html += HSDConfig::FLICKERING; html += F("'>Flicker</option>");
  
    return html;
}

// ---------------------------------------------------------------------------------------------------------------------
#ifdef HSD_SENSOR_ENABLED
void HSDWebserver::setSensorData(float& temp, float& hum) {
    m_lastTemp = temp;
    m_lastHum = hum;
}
#endif // HSD_SENSOR_ENABLED  
