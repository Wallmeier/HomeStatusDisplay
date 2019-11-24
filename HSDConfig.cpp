#include "HSDConfig.hpp"

#include <ArduinoJson.h>
#ifdef ARDUINO_ARCH_ESP32
#include <SPIFFS.h>
#else
#include <FS.h>
#endif

#define FILENAME_COLORMAPPING "/colormapping.json"
#define FILENAME_DEVMAPPING   "/devicemapping.json"
#define FILENAME_MAINCONFIG   "/config.json"

const constexpr HSDConfig::Map HSDConfig::DefaultColor[];

HSDConfig::HSDConfig() :
    m_cfgEntries(nullptr),
#if defined HSD_BLUETOOTH_ENABLED && defined ARDUINO_ARCH_ESP32
    m_cfgBluetoothEnabled(false),
#endif
#ifdef HSD_CLOCK_ENABLED
    m_cfgClockBrightness(4),
    m_cfgClockEnabled(false),
    m_cfgClockNTPInterval(20),
    m_cfgClockNTPServer("pool.ntp.org"),
    m_cfgClockPinCLK(0),
    m_cfgClockPinDIO(0),
    m_cfgClockTimeZone("CET-1CEST,M3.5.0/2,M10.5.0/3"),
#endif // HSD_CLOCK_ENABLED
    m_cfgColorMapping(MAX_COLOR_MAP_ENTRIES),
    m_cfgColorMappingDirty(true),
    m_cfgDeviceMapping(MAX_DEVICE_MAP_ENTRIES),
    m_cfgDeviceMappingDirty(true),
    m_cfgHost("HomeStatusDisplay"),
    m_cfgLedBrightness(50),
    m_cfgLedDataPin(0),
    m_cfgMqttPort(1883),
    m_cfgNumberOfLeds(0),
#ifdef HSD_SENSOR_ENABLED
    m_cfgSensorSonoffEnabled(false),
    m_cfgSensorInterval(2),
    m_cfgSensorPin(0)
#endif // HSD_SENSOR_ENABLED
{
    int len(13);
#ifdef MQTT_TEST_TOPIC
    len += 1;
#endif    
#if defined HSD_BLUETOOTH_ENABLED && defined ARDUINO_ARCH_ESP32
    len += 1;
#endif
#ifdef HSD_SENSOR_ENABLED
    len += 3;
#endif
#ifdef HSD_CLOCK_ENABLED
    len += 7;
#endif
    int idx(0);
    m_cfgEntries = new ConfigEntry[len];
    m_cfgEntries[idx++] = {Group::Wifi,      F("wifiHost"),        F("Hostname"),                      F("host"),                         DataType::String, 0, false, &m_cfgHost};
    m_cfgEntries[idx++] = {Group::Wifi,      F("wifiSSID"),        F("SSID"),                          F("SSID"),                         DataType::String, 0, false, &m_cfgWifiSSID};
    m_cfgEntries[idx++] = {Group::Wifi,      F("wifiPSK"),         F("Password"),                      F("Password"),                     DataType::String, 0, true,  &m_cfgWifiPSK};
    m_cfgEntries[idx++] = {Group::Mqtt,      F("mqttServer"),      F("Server"),                        F("IP or hostname"),               DataType::String, 0, false, &m_cfgMqttServer};
    m_cfgEntries[idx++] = {Group::Mqtt,      F("mqttPort"),        F("Port"),                          F("Port"),                         DataType::Word,   5, false, &m_cfgMqttPort};
    m_cfgEntries[idx++] = {Group::Mqtt,      F("mqttUser"),        F("User"),                          F("User name"),                    DataType::String, 0, false, &m_cfgMqttUser};
    m_cfgEntries[idx++] = {Group::Mqtt,      F("mqttPassword"),    F("Password"),                      F("Password"),                     DataType::String, 0, true,  &m_cfgMqttPassword};
    m_cfgEntries[idx++] = {Group::Mqtt,      F("mqttStatusTopic"), F("Status topic"),                  F("#"),                            DataType::String, 0, false, &m_cfgMqttStatusTopic};
#ifdef MQTT_TEST_TOPIC
    m_cfgEntries[idx++] = {Group::Mqtt,      F("mqttTestTopic"),   F("Test topic"),                    F("#"),                            DataType::String, 0, false, &m_cfgMqttTestTopic};
#endif // MQTT_TEST_TOPIC        
    m_cfgEntries[idx++] = {Group::Mqtt,      F("mqttOutTopic"),    F("Outgoing topic"),                F("#"),                            DataType::String, 0, false, &m_cfgMqttOutTopic};
    m_cfgEntries[idx++] = {Group::Leds,      F("ledCount"),        F("Number of LEDs"),                F("0"),                            DataType::Byte,   3, false, &m_cfgNumberOfLeds};
    m_cfgEntries[idx++] = {Group::Leds,      F("ledPin"),          F("LED pin"),                       F("0"),                            DataType::Byte,   3, false, &m_cfgLedDataPin};
    m_cfgEntries[idx++] = {Group::Leds,      F("ledBrightness"),   F("Brightness"),                    F("0-255"),                        DataType::Byte,   3, false, &m_cfgLedBrightness};
#ifdef HSD_CLOCK_ENABLED
    m_cfgEntries[idx++] = {Group::Clock,     F("clockEnabled"),    F("Enable"),                        nullptr,                           DataType::Bool,   0, false, &m_cfgClockEnabled};
    m_cfgEntries[idx++] = {Group::Clock,     F("clockCLK"),        F("CLK pin"),                       F("0"),                            DataType::Byte,   3, false, &m_cfgClockPinCLK};
    m_cfgEntries[idx++] = {Group::Clock,     F("clockDIO"),        F("DIO pin"),                       F("0"),                            DataType::Byte,   3, false, &m_cfgClockPinDIO};
    m_cfgEntries[idx++] = {Group::Clock,     F("clockBrightness"), F("Brightness"),                    F("0-8"),                          DataType::Byte,   1, false, &m_cfgClockBrightness};
    m_cfgEntries[idx++] = {Group::Clock,     F("clockTZ"),         F("Time zone"),                     F("CET-1CEST,M3.5.0/2,M10.5.0/3"), DataType::String, 0, false, &m_cfgClockTimeZone};
    m_cfgEntries[idx++] = {Group::Clock,     F("clockServer"),     F("NTP server"),                    F("pool.ntp.org"),                 DataType::String, 0, false, &m_cfgClockNTPServer};
    m_cfgEntries[idx++] = {Group::Clock,     F("clockInterval"),   F("NTP update interval (min.)"),    F("20"),                           DataType::Word,   5, false, &m_cfgClockNTPInterval};
#endif // HSD_CLOCK_ENABLED
#ifdef HSD_SENSOR_ENABLED
    m_cfgEntries[idx++] = {Group::Sensors,   F("sensorEnabled"),   F("Sonoff SI7021"),                 nullptr,                           DataType::Bool,   0, false, &m_cfgSensorSonoffEnabled};
    m_cfgEntries[idx++] = {Group::Sensors,   F("sensorPin"),       F("Data pin"),                      F("0"),                            DataType::Byte,   0, false, &m_cfgSensorPin};
    m_cfgEntries[idx++] = {Group::Sensors,   F("sensorInterval"),  F("Sensor update interval (min.)"), F("5"),                            DataType::Word,   5, false, &m_cfgSensorInterval};
#endif // HSD_SENSOR_ENABLED        
#if defined HSD_BLUETOOTH_ENABLED && defined ARDUINO_ARCH_ESP32
    m_cfgEntries[idx++] = {Group::Bluetooth, F("btEnabled"),       F("Enable"),                        nullptr,                           DataType::Bool,   0, false, &m_cfgBluetoothEnabled};
#endif // HSD_BLUETOOTH_ENABLED
    m_cfgEntries[idx++] = {Group::_Last,     nullptr,              nullptr,                            nullptr,                           DataType::String, 0, false, nullptr};
}

// ---------------------------------------------------------------------------------------------------------------------

void HSDConfig::begin() {
    Serial.println(F(""));
    Serial.println(F("Initializing config."));

    if (SPIFFS.begin()) {
        Serial.println(F("Mounted file system."));
        readMainConfigFile();
        readColorMappingConfigFile();
        readDeviceMappingConfigFile();
    } else {
        Serial.println(F("Failed to mount file system"));
    }
}

// ---------------------------------------------------------------------------------------------------------------------

bool HSDConfig::readFile(const String& fileName, String& content) const {
    bool success(false);
    Serial.print(F("Reading config file ")); Serial.println(fileName);
    if (SPIFFS.exists(fileName)) {
        File configFile = SPIFFS.open(fileName, "r");
        if (configFile) {
            size_t size = configFile.size();
            Serial.print(F("File size is "));
            Serial.println(String(size) + " bytes");
            char* buffer = new char[size + 1];
            buffer[size] = 0;
            configFile.readBytes(buffer, size);
            content = buffer;
            delete[] buffer;
            success = true;
        } else {
            Serial.println(F("File open failed"));
        }
        configFile.close();
    } else {
        Serial.println(F("File does not exist"));
    }
    return success;
}

// ---------------------------------------------------------------------------------------------------------------------

bool HSDConfig::readMainConfigFile() {
    bool success(false);
    String fileBuffer;

    if (readFile(FILENAME_MAINCONFIG, fileBuffer)) {
        DynamicJsonBuffer jsonBuffer(fileBuffer.length() + 1);
        JsonObject& json = jsonBuffer.parseObject(fileBuffer);
        if (json.success()) {
            Serial.println(F("Main config data successfully parsed."));
            Serial.print(F("JSON length is ")); Serial.println(json.measureLength());
            printMainConfigFile(json);
            Serial.println(F(""));
            int idx(0);
            do {
                if (json.containsKey(m_cfgEntries[idx].key)) {
                    switch (m_cfgEntries[idx].type) {
                        case DataType::String: *(reinterpret_cast<String*> ( m_cfgEntries[idx].val)) = json[m_cfgEntries[idx].key].as<String>(); break;
                        case DataType::Bool:   *(reinterpret_cast<bool*>(    m_cfgEntries[idx].val)) = json[m_cfgEntries[idx].key].as<bool>();   break;
                        case DataType::Byte:   *(reinterpret_cast<uint8_t*>( m_cfgEntries[idx].val)) = json[m_cfgEntries[idx].key].as<int>();    break;
                        case DataType::Word:   *(reinterpret_cast<uint16_t*>(m_cfgEntries[idx].val)) = json[m_cfgEntries[idx].key].as<int>();    break;
                    }
                }
            } while (m_cfgEntries[++idx].group != Group::_Last); 
            success = true;
        } else {
            Serial.println(F("Could not parse config data."));
        }
    } else {
        Serial.println(F("Creating default main config file."));
        writeMainConfigFile();
    }
    return success;
}

// ---------------------------------------------------------------------------------------------------------------------

void HSDConfig::printMainConfigFile(JsonObject& json) {
    int idx(0), maxLen(0);
    do {
        int length = strlen_P((PGM_P)m_cfgEntries[idx].key);
        if (length > maxLen)
            maxLen = length;
    } while (m_cfgEntries[++idx].group != Group::_Last);
    idx = 0;
    do {
        Serial.print(F("  • "));
        Serial.print(m_cfgEntries[idx].key);
        int length = strlen_P((PGM_P)m_cfgEntries[idx].key);
        while (length < maxLen) {
            length++;
            Serial.print(" ");
        }
        Serial.print(F(": "));
        if (m_cfgEntries[idx].doNotDump)
            Serial.println("not shown");
        else
            Serial.println((const char*)(json[m_cfgEntries[idx].key]));
    } while (m_cfgEntries[++idx].group != Group::_Last);
}

// ---------------------------------------------------------------------------------------------------------------------

bool HSDConfig::readColorMappingConfigFile() {
    bool success(false);
    String fileBuffer;
    if (readFile(FILENAME_COLORMAPPING, fileBuffer)) {
        DynamicJsonBuffer jsonBuffer(fileBuffer.length() + 1);
        JsonObject& json = jsonBuffer.parseObject(fileBuffer);
        if (json.success()) {
            Serial.println(F("Color mapping config data successfully parsed."));
            Serial.print(F("JSON length is ")); Serial.println(json.measureLength());
            //json.prettyPrintTo(Serial);
            Serial.println(F(""));

            success = true;
            int index = 0;

            for(JsonObject::iterator it = json.begin(); it != json.end(); ++it) {
                JsonObject& entry = json[it->key];

                if (entry.containsKey(JSON_KEY_COLORMAPPING_MSG) && entry.containsKey(JSON_KEY_COLORMAPPING_COLOR) &&
                    entry.containsKey(JSON_KEY_COLORMAPPING_BEHAVIOR)) {
                    addColorMappingEntry(index,
                                         entry[JSON_KEY_COLORMAPPING_MSG].as<char*>(),
                                         entry[JSON_KEY_COLORMAPPING_COLOR].as<uint32_t>(),
                                         (Behavior)(entry[JSON_KEY_COLORMAPPING_BEHAVIOR].as<int>()));
                    index++;
                }
            }
        } else {
            Serial.println(F("Could not parse config data."));
        }
    } else {
        Serial.println(F("Creating default color mapping config file."));
        writeColorMappingConfigFile();
    }

    // load default color mapping
    if (!success || m_cfgColorMapping.size() == 0) {
        for (uint8_t i = 1; i < NUMBER_OF_DEFAULT_COLORS; i++) {
            String temp = String(DefaultColor[i].key);
            temp.toLowerCase();
            addColorMappingEntry(i - 1, temp, DefaultColor[i].value, Behavior::On);
        }
    }

    m_cfgColorMappingDirty = false;
}

// ---------------------------------------------------------------------------------------------------------------------

bool HSDConfig::readDeviceMappingConfigFile() {
    bool success(false);
    String fileBuffer;
    if (readFile(FILENAME_DEVMAPPING, fileBuffer)) {
        DynamicJsonBuffer jsonBuffer(fileBuffer.length() + 1);
        JsonObject& json = jsonBuffer.parseObject(fileBuffer);

        if (json.success()) {
            Serial.println(F("Device mapping config data successfully parsed."));
            Serial.print(F("JSON length is "));
            Serial.println(json.measureLength());
            //json.prettyPrintTo(Serial);
            Serial.println(F(""));

            success = true;
            int index(0);

            for (JsonObject::iterator it = json.begin(); it != json.end(); ++it) {
                JsonObject& entry = json[it->key];
                if (entry.containsKey(JSON_KEY_DEVICEMAPPING_NAME) && entry.containsKey(JSON_KEY_DEVICEMAPPING_LED)) {
                    addDeviceMappingEntry(index,
                                          entry[JSON_KEY_DEVICEMAPPING_NAME].as<char*>(),
                                          entry[JSON_KEY_DEVICEMAPPING_LED].as<int>());
                    index++;
                }
            }
        } else {
            Serial.println(F("Could not parse config data."));
        }
    } else {
        Serial.println(F("Creating default device mapping config file."));
        writeDeviceMappingConfigFile();
    }

    m_cfgDeviceMappingDirty = false;
}

// ---------------------------------------------------------------------------------------------------------------------

bool HSDConfig::writeFile(const String& fileName, JsonObject* data) const {
    bool success(false);

    Serial.print(F("Writing config file "));
    Serial.println(fileName);

    File configFile = SPIFFS.open(fileName, "w+");
    if (configFile) {
        data->printTo(configFile);
        configFile.close();
        success = true;
    } else {
        Serial.println(F("File open failed"));
    }
    return success;
}

// ---------------------------------------------------------------------------------------------------------------------

void HSDConfig::writeMainConfigFile() const {
    DynamicJsonBuffer jsonBuffer;
    JsonObject& json = jsonBuffer.createObject();
    int idx(0);
    do {
        switch (m_cfgEntries[idx].type) {
            case DataType::String: json[m_cfgEntries[idx].key] = *(reinterpret_cast<String*> ( m_cfgEntries[idx].val)); break;
            case DataType::Bool:   json[m_cfgEntries[idx].key] = *(reinterpret_cast<bool*>(    m_cfgEntries[idx].val)); break;
            case DataType::Byte:   json[m_cfgEntries[idx].key] = *(reinterpret_cast<uint8_t*>( m_cfgEntries[idx].val)); break;
            case DataType::Word:   json[m_cfgEntries[idx].key] = *(reinterpret_cast<uint16_t*>(m_cfgEntries[idx].val)); break;
        }
    } while (m_cfgEntries[++idx].group != Group::_Last); 
    if (!writeFile(FILENAME_MAINCONFIG, &json))
        onFileWriteError();
}

// ---------------------------------------------------------------------------------------------------------------------

void HSDConfig::writeColorMappingConfigFile() {
    DynamicJsonBuffer jsonBuffer;
    JsonObject& json = jsonBuffer.createObject();

    for (int index = 0; index < m_cfgColorMapping.size(); index++) {
        const ColorMapping* mapping = m_cfgColorMapping.get(index);
        if (strlen(mapping->msg) != 0) {
            Serial.print(F("Preparing to write color mapping config file index "));
            Serial.print(String(index));
            Serial.print(F(", msg="));
            Serial.println(String(mapping->msg));

            JsonObject& colorMappingEntry = json.createNestedObject(String(index));

            colorMappingEntry[JSON_KEY_COLORMAPPING_MSG] = mapping->msg;
            colorMappingEntry[JSON_KEY_COLORMAPPING_COLOR] = mapping->color;
            colorMappingEntry[JSON_KEY_COLORMAPPING_BEHAVIOR] = (int)mapping->behavior;
        } else {
            Serial.print(F("Removing color mapping config file index "));
            Serial.println(String(index));
        }
    }

    if (!writeFile(FILENAME_COLORMAPPING, &json))
        onFileWriteError();
    else
        m_cfgColorMappingDirty = false;
}

// ---------------------------------------------------------------------------------------------------------------------

void HSDConfig::writeDeviceMappingConfigFile() {
    DynamicJsonBuffer jsonBuffer;
    JsonObject& json = jsonBuffer.createObject();

    for (int index = 0; index < m_cfgDeviceMapping.size(); index++) {
        const DeviceMapping* mapping = m_cfgDeviceMapping.get(index);

        if (strlen(mapping->name) != 0) {
            Serial.print(F("Preparing to write device mapping config file index "));
            Serial.println(String(index));

            JsonObject& deviceMappingEntry = json.createNestedObject(String(index));

            deviceMappingEntry[JSON_KEY_DEVICEMAPPING_NAME] = mapping->name;
            deviceMappingEntry[JSON_KEY_DEVICEMAPPING_LED]  = (int)mapping->ledNumber;
        } else {
            Serial.print(F("Removing device mapping config file index "));
            Serial.println(String(index));
        }
    }

    if (!writeFile(FILENAME_DEVMAPPING, &json))
        onFileWriteError();
    else
        m_cfgDeviceMappingDirty = false;
}

// ---------------------------------------------------------------------------------------------------------------------

void HSDConfig::onFileWriteError() const {
    Serial.println(F("Failed to write file, formatting file system."));
    SPIFFS.format();
    Serial.println(F("Done."));
}

// ---------------------------------------------------------------------------------------------------------------------

bool HSDConfig::addDeviceMappingEntry(int entryNum, String name, int ledNumber) {
    bool success(false);

    Serial.print(F("Adding or editing device mapping entry at index "));
    Serial.println(String(entryNum) + " with name " + name + ", LED number " + String(ledNumber));

    DeviceMapping mapping(name, ledNumber);

    if (m_cfgDeviceMapping.set(entryNum, mapping)) {
        m_cfgDeviceMappingDirty = true;
        success = true;
    } else {
        Serial.println(F("Cannot add/edit device mapping entry"));
    }

    return success;
}

// ---------------------------------------------------------------------------------------------------------------------

bool HSDConfig::deleteDeviceMappingEntry(int entryNum) {
    bool removed(m_cfgDeviceMapping.remove(entryNum));
    if (removed)
        m_cfgDeviceMappingDirty = true;
    return removed;
}

// ---------------------------------------------------------------------------------------------------------------------

void HSDConfig::deleteAllDeviceMappingEntries() {
    m_cfgDeviceMapping.clear();
    m_cfgDeviceMappingDirty = true;
}

// ---------------------------------------------------------------------------------------------------------------------

bool HSDConfig::addColorMappingEntry(int entryNum, String msg, uint32_t color, Behavior behavior) {
    bool success(false);

    Serial.print(F("Adding or editing color mapping entry at index "));
    Serial.println(String(entryNum) + ", new values: name " + msg + ", color " + String(color) + ", behavior " + String(static_cast<uint8_t>(behavior)));

    ColorMapping mapping(msg, color, behavior);

    if (m_cfgColorMapping.set(entryNum, mapping)) {
        m_cfgColorMappingDirty = true;
        success = true;
    } else {
        Serial.println(F("Cannot add/edit device mapping entry"));
    }

    return success;
}

// ---------------------------------------------------------------------------------------------------------------------

bool HSDConfig::deleteColorMappingEntry(int entryNum) {
    bool removed(m_cfgColorMapping.remove(entryNum));
    if (removed)
        m_cfgColorMappingDirty = true;
    return removed;
}

// ---------------------------------------------------------------------------------------------------------------------

void HSDConfig::deleteAllColorMappingEntries() {
    m_cfgColorMapping.clear();
    m_cfgColorMappingDirty = true;
}

// ---------------------------------------------------------------------------------------------------------------------

int HSDConfig::getLedNumber(const String& deviceName) const {
    for (int i = 0; i < m_cfgDeviceMapping.size(); i++) {
        const DeviceMapping* mapping = m_cfgDeviceMapping.get(i);
        if (deviceName.equals(mapping->name))
            return mapping->ledNumber;
    }
    return -1;
}

// ---------------------------------------------------------------------------------------------------------------------

String HSDConfig::getDevice(int ledNumber) const {
    for (int i = 0; i < m_cfgDeviceMapping.size(); i++) {
        const DeviceMapping* mapping = m_cfgDeviceMapping.get(i);
        if (ledNumber == mapping->ledNumber)
            return mapping->name;
    }
    return "";
}

// ---------------------------------------------------------------------------------------------------------------------

int HSDConfig::getColorMapIndex(const String& msg) const {
    for (int i = 0; i < m_cfgColorMapping.size(); i++) {
        const ColorMapping* mapping = m_cfgColorMapping.get(i);
        if (msg.equals(mapping->msg))
            return i;
    }
    return -1;
}

// ---------------------------------------------------------------------------------------------------------------------

uint32_t HSDConfig::getDefaultColor(String key) const {
    key.toUpperCase();
    for (uint8_t i = 0; i < NUMBER_OF_DEFAULT_COLORS; i++) {
        if (String(HSDConfig::DefaultColor[i].key) == key)
            return HSDConfig::DefaultColor[i].value;
    }
    return 0;
}

// ---------------------------------------------------------------------------------------------------------------------

String HSDConfig::getDefaultColor(uint32_t value) const {
    for (uint8_t i = 0; i < NUMBER_OF_DEFAULT_COLORS; i++) {
        if (HSDConfig::DefaultColor[i].value == value)
            return String(HSDConfig::DefaultColor[i].key);
    }
    return "";
}

// ---------------------------------------------------------------------------------------------------------------------

String HSDConfig::hex2string(uint32_t value) const {
    char buf[6];
    sprintf(buf, "%06X", value);
    return "#" + String(buf);
}

// ---------------------------------------------------------------------------------------------------------------------

uint32_t HSDConfig::string2hex(String value) const {
    value.trim();
    value.replace("%23", "");
    value.replace("#", "");
    return strtoul(value.c_str(), nullptr, 16);
}

// ---------------------------------------------------------------------------------------------------------------------

String HSDConfig::getMqttOutTopic(const String& topic) const {
    if (m_cfgMqttOutTopic.length() == 0)
        return "";
    else if (m_cfgMqttOutTopic.endsWith("/"))
        return m_cfgMqttOutTopic + topic;
    else
        return m_cfgMqttOutTopic + "/" + topic;
}

// ---------------------------------------------------------------------------------------------------------------------

String HSDConfig::groupDescription(Group group) const {
    switch (group) {
        case Group::Wifi:      return F("WiFi");
        case Group::Mqtt:      return F("MQTT");
        case Group::Leds:      return F("LEDs");
        case Group::Clock:     return F("Clock");
        case Group::Sensors:   return F("Sensors");
        case Group::Bluetooth: return F("Bluetooth");        
    }
}
