#include "HSDConfig.hpp"

#include <ArduinoJson.h>
#ifdef ARDUINO_ARCH_ESP32
#include <SPIFFS.h>
#else
#include <FS.h>
#endif

#define JSON_KEY_COLORMAPPING          (F("colorMapping"))
#define JSON_KEY_COLORMAPPING_MSG      (F("message"))
#define JSON_KEY_COLORMAPPING_COLOR    (F("color"))
#define JSON_KEY_COLORMAPPING_BEHAVIOR (F("behavior"))
#define JSON_KEY_DEVICEMAPPING         (F("deviceMapping"))
#define JSON_KEY_DEVICEMAPPING_DEVICE  (F("device"))
#define JSON_KEY_DEVICEMAPPING_LED     (F("led"))

const constexpr HSDConfig::Map HSDConfig::DefaultColor[];

HSDConfig::HSDConfig() :
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
    m_cfgColorMappingDirty(false),
    m_cfgDeviceMappingDirty(false),
    m_cfgHost("HomeStatusDisplay"),
    m_cfgLedBrightness(50),
    m_cfgLedDataPin(0),
    m_cfgMqttPort(1883),
    m_cfgNumberOfLeds(0),
#ifdef HSD_SENSOR_ENABLED
    m_cfgSensorI2CEnabled(false),
    m_cfgSensorInterval(2),
    m_cfgSensorPin(0),
    m_cfgSensorSonoffEnabled(false),
    m_cfgSensorAltitude(0)
#endif // HSD_SENSOR_ENABLED
{
    m_cfgEntries.push_back({Group::Wifi,      F("host"),          F("Hostname"),                      F("host"),                         DataType::String,   0, &m_cfgHost});
    m_cfgEntries.push_back({Group::Wifi,      F("SSID"),          F("SSID"),                          F("SSID"),                         DataType::String,   0, &m_cfgWifiSSID});
    m_cfgEntries.push_back({Group::Wifi,      F("PSK"),           F("Password"),                      F("Password"),                     DataType::Password, 0, &m_cfgWifiPSK});
    m_cfgEntries.push_back({Group::Mqtt,      F("server"),        F("Server"),                        F("IP or hostname"),               DataType::String,   0, &m_cfgMqttServer});
    m_cfgEntries.push_back({Group::Mqtt,      F("port"),          F("Port"),                          F("Port"),                         DataType::Word,     5, &m_cfgMqttPort});
    m_cfgEntries.push_back({Group::Mqtt,      F("user"),          F("User"),                          F("User name"),                    DataType::String,   0, &m_cfgMqttUser});
    m_cfgEntries.push_back({Group::Mqtt,      F("password"),      F("Password"),                      F("Password"),                     DataType::Password, 0, &m_cfgMqttPassword});
    m_cfgEntries.push_back({Group::Mqtt,      F("statusTopic"),   F("Status topic"),                  F("#"),                            DataType::String,   0, &m_cfgMqttStatusTopic});
#ifdef MQTT_TEST_TOPIC
    m_cfgEntries.push_back({Group::Mqtt,      F("testTopic"),     F("Test topic"),                    F("#"),                            DataType::String,   0, &m_cfgMqttTestTopic});
#endif // MQTT_TEST_TOPIC
    m_cfgEntries.push_back({Group::Mqtt,      F("outTopic"),      F("Outgoing topic"),                F("#"),                            DataType::String,   0, &m_cfgMqttOutTopic});
    m_cfgEntries.push_back({Group::Leds,      F("count"),         F("Number of LEDs"),                F("0"),                            DataType::Byte,     3, &m_cfgNumberOfLeds});
    m_cfgEntries.push_back({Group::Leds,      F("pin"),           F("LED pin"),                       F("0"),                            DataType::Byte,     2, &m_cfgLedDataPin});
    m_cfgEntries.push_back({Group::Leds,      F("brightness"),    F("Brightness"),                    F("0-255"),                        DataType::Byte,     3, &m_cfgLedBrightness});
#ifdef HSD_CLOCK_ENABLED
    m_cfgEntries.push_back({Group::Clock,     F("enabled"),       F("Enable"),                        nullptr,                           DataType::Bool,     0, &m_cfgClockEnabled});
    m_cfgEntries.push_back({Group::Clock,     F("CLK"),           F("CLK pin"),                       F("0"),                            DataType::Byte,     2, &m_cfgClockPinCLK});
    m_cfgEntries.push_back({Group::Clock,     F("DIO"),           F("DIO pin"),                       F("0"),                            DataType::Byte,     2, &m_cfgClockPinDIO});
    m_cfgEntries.push_back({Group::Clock,     F("brightness"),    F("Brightness"),                    F("0-8"),                          DataType::Byte,     1, &m_cfgClockBrightness});
    m_cfgEntries.push_back({Group::Clock,     F("timezone"),      F("Time zone"),                     F("CET-1CEST,M3.5.0/2,M10.5.0/3"), DataType::String,   0, &m_cfgClockTimeZone});
    m_cfgEntries.push_back({Group::Clock,     F("server"),        F("NTP server"),                    F("pool.ntp.org"),                 DataType::String,   0, &m_cfgClockNTPServer});
    m_cfgEntries.push_back({Group::Clock,     F("interval"),      F("NTP update interval (min.)"),    F("20"),                           DataType::Word,     5, &m_cfgClockNTPInterval});
#endif // HSD_CLOCK_ENABLED
#ifdef HSD_SENSOR_ENABLED
    m_cfgEntries.push_back({Group::Sensors,   F("sonoffEnabled"), F("Sonoff SI7021"),                 nullptr,                           DataType::Bool,     0, &m_cfgSensorSonoffEnabled});
    m_cfgEntries.push_back({Group::Sensors,   F("sonoffPin"),     F("Data pin"),                      F("0"),                            DataType::Byte,     2, &m_cfgSensorPin});
    m_cfgEntries.push_back({Group::Sensors,   F("interval"),      F("Sensor update interval (min.)"), F("5"),                            DataType::Word,     5, &m_cfgSensorInterval});
    m_cfgEntries.push_back({Group::Sensors,   F("i2cEnabled"),    F("I2C"),                           nullptr,                           DataType::Bool,     0, &m_cfgSensorI2CEnabled});
    m_cfgEntries.push_back({Group::Sensors,   F("altitude"),      F("Altitude"),                      F("0"),                            DataType::Word,     5, &m_cfgSensorAltitude});
#endif // HSD_SENSOR_ENABLED
#if defined HSD_BLUETOOTH_ENABLED && defined ARDUINO_ARCH_ESP32
    m_cfgEntries.push_back({Group::Bluetooth, F("enabled"),       F("Enable"),                        nullptr,                           DataType::Bool,     0, &m_cfgBluetoothEnabled});
#endif // HSD_BLUETOOTH_ENABLED
}

// ---------------------------------------------------------------------------------------------------------------------

void HSDConfig::begin() {
    Serial.printf("\nInitializing config (%u entries) - using ArduinoJson version %s\n", m_cfgEntries.size(), ARDUINOJSON_VERSION);

    if (SPIFFS.begin()) {
        Serial.println(F("Mounted file system."));
        readConfigFile();
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

bool HSDConfig::readConfigFile() {
    bool success(false);
    String fileBuffer;

    if (readFile(FILENAME_MAINCONFIG, fileBuffer)) {
        DynamicJsonBuffer jsonBuffer(fileBuffer.length() + 1);
        JsonObject& root = jsonBuffer.parseObject(fileBuffer);
        if (root.success()) {
            Serial.println(F("Config data successfully parsed."));
            int maxLen(0), len(0);
            for (size_t idx = 0; idx < m_cfgEntries.size(); idx++) {
                len = strlen_P((PGM_P)m_cfgEntries[idx].key) + groupDescription(m_cfgEntries[idx].group).length() + 1;
                if (len > maxLen)
                    maxLen = len;
            }

            Group prevGroup = Group::__Last;
            JsonObject* json = nullptr;
            String groupName;
            for (size_t idx = 0; idx < m_cfgEntries.size(); idx++) {
                if (prevGroup != m_cfgEntries[idx].group) {
                    prevGroup = m_cfgEntries[idx].group;
                    groupName = groupDescription(prevGroup);
                    groupName.toLowerCase();
                    if (root.containsKey(groupName))
                        json = &root[groupName].as<JsonObject>();
                    else
                        json = nullptr;
                }

                Serial.print(F("  • "));
                Serial.print(groupName);
                Serial.print(F("."));
                Serial.print(m_cfgEntries[idx].key);
                for (int i = groupName.length() + 1 + strlen_P((PGM_P)m_cfgEntries[idx].key); i < maxLen; i++)
                    Serial.print(F(" "));
                Serial.print(F(": "));
                if (m_cfgEntries[idx].type == DataType::Password) {
                    Serial.println("not shown");
                } else if (json) {
                    Serial.println((const char*)((*json)[m_cfgEntries[idx].key]));
                }  else {
                    Serial.println(F(""));
                }
                if (json && json->containsKey(m_cfgEntries[idx].key)) {
                    switch (m_cfgEntries[idx].type) {
                        case DataType::Password:
                        case DataType::String: *(reinterpret_cast<String*> ( m_cfgEntries[idx].val)) = (*json)[m_cfgEntries[idx].key].as<String>(); break;
                        case DataType::Bool:   *(reinterpret_cast<bool*>(    m_cfgEntries[idx].val)) = (*json)[m_cfgEntries[idx].key].as<bool>();   break;
                        case DataType::Byte:   *(reinterpret_cast<uint8_t*>( m_cfgEntries[idx].val)) = (*json)[m_cfgEntries[idx].key].as<int>();    break;
                        case DataType::Word:   *(reinterpret_cast<uint16_t*>(m_cfgEntries[idx].val)) = (*json)[m_cfgEntries[idx].key].as<int>();    break;
                    }
                }
            } 
            
            if (root.containsKey(JSON_KEY_COLORMAPPING)) {
                JsonArray& colMap = root[JSON_KEY_COLORMAPPING].as<JsonArray>();
                int index(0);
                for (size_t idx = 0; idx < colMap.size(); idx++) {
                    JsonObject& elem = colMap.get<JsonVariant>(idx).as<JsonObject>();
                    if (elem.containsKey(JSON_KEY_COLORMAPPING_MSG) && elem.containsKey(JSON_KEY_COLORMAPPING_COLOR) &&
                        elem.containsKey(JSON_KEY_COLORMAPPING_BEHAVIOR)) {
                        addColorMappingEntry(index, elem[JSON_KEY_COLORMAPPING_MSG].as<char*>(),
                                             elem[JSON_KEY_COLORMAPPING_COLOR].as<uint32_t>(),
                                            (Behavior)(elem[JSON_KEY_COLORMAPPING_BEHAVIOR].as<int>()));
                        index++;
                    }
                }
                m_cfgColorMappingDirty = false;
            }
            
            if (root.containsKey(JSON_KEY_DEVICEMAPPING)) {
                JsonArray& devMap = root[JSON_KEY_DEVICEMAPPING].as<JsonArray>();
                int index(0);
                for (size_t idx = 0; idx < devMap.size(); idx++) {
                    JsonObject& elem = devMap.get<JsonVariant>(idx).as<JsonObject>();
                    if (elem.containsKey(JSON_KEY_DEVICEMAPPING_DEVICE) && elem.containsKey(JSON_KEY_DEVICEMAPPING_LED)) {
                        addDeviceMappingEntry(index, elem[JSON_KEY_DEVICEMAPPING_DEVICE].as<char*>(),
                                              elem[JSON_KEY_DEVICEMAPPING_LED].as<int>());
                        index++;
                    }
                }
                m_cfgDeviceMappingDirty = false;
            }
            
            success = true;
        } else {
            Serial.println(F("Could not parse config data."));
        }
    } else {
        Serial.println(F("Creating default main config file."));
        writeConfigFile();
    }
    return success;
}

// ---------------------------------------------------------------------------------------------------------------------

void HSDConfig::writeConfigFile() const {
    DynamicJsonBuffer jsonBuffer;
    JsonObject& root = jsonBuffer.createObject();
    Group prevGroup = Group::__Last;
    JsonObject* json = nullptr;
    for (size_t idx = 0; idx < m_cfgEntries.size(); idx++) {
        if (prevGroup != m_cfgEntries[idx].group) {
            prevGroup = m_cfgEntries[idx].group;
            String groupName = groupDescription(prevGroup);
            groupName.toLowerCase();
            json = &root.createNestedObject(groupName);
        }
        switch (m_cfgEntries[idx].type) {
            case DataType::Password:
            case DataType::String: (*json)[m_cfgEntries[idx].key] = *(reinterpret_cast<String*> ( m_cfgEntries[idx].val)); break;
            case DataType::Bool:   (*json)[m_cfgEntries[idx].key] = *(reinterpret_cast<bool*>(    m_cfgEntries[idx].val)); break;
            case DataType::Byte:   (*json)[m_cfgEntries[idx].key] = *(reinterpret_cast<uint8_t*>( m_cfgEntries[idx].val)); break;
            case DataType::Word:   (*json)[m_cfgEntries[idx].key] = *(reinterpret_cast<uint16_t*>(m_cfgEntries[idx].val)); break;
        }
    }

    JsonArray& colMapping = root.createNestedArray(JSON_KEY_COLORMAPPING);
    for (unsigned int index = 0; index < m_cfgColorMapping.size(); index++) {
        const ColorMapping& mapping = m_cfgColorMapping[index];
        if (mapping.msg.length() > 0) {
            JsonObject& colorMappingEntry = colMapping.createNestedObject(); 
            colorMappingEntry[JSON_KEY_COLORMAPPING_MSG] = mapping.msg;
            colorMappingEntry[JSON_KEY_COLORMAPPING_COLOR] = mapping.color;
            colorMappingEntry[JSON_KEY_COLORMAPPING_BEHAVIOR] = static_cast<int>(mapping.behavior);
        }
    }

    JsonArray& devMapping = root.createNestedArray(JSON_KEY_DEVICEMAPPING);
    for (int index = 0; index < m_cfgDeviceMapping.size(); index++) {
        const DeviceMapping& mapping = m_cfgDeviceMapping[index];
        if (mapping.device.length() > 0) {
            JsonObject& deviceMappingEntry = devMapping.createNestedObject();
            deviceMappingEntry[JSON_KEY_DEVICEMAPPING_DEVICE] = mapping.device;
            deviceMappingEntry[JSON_KEY_DEVICEMAPPING_LED]  = static_cast<int>(mapping.ledNumber);
        }
    }

    Serial.print(F("Writing config file "));
    Serial.println(FILENAME_MAINCONFIG);
    File configFile = SPIFFS.open(FILENAME_MAINCONFIG, "w+");
    if (configFile) {
        root.prettyPrintTo(configFile);
        configFile.close();
    } else {
        Serial.println(F("Failed to write file, formatting file system."));
        SPIFFS.format();
        Serial.println(F("Done."));
    }
}

// ---------------------------------------------------------------------------------------------------------------------

void HSDConfig::addDeviceMappingEntry(int entryNum, String name, int ledNumber) {
    Serial.print(F("Adding or editing device mapping entry at index "));
    Serial.println(String(entryNum) + " with name " + name + ", LED number " + String(ledNumber));

    DeviceMapping mapping(name, ledNumber);

    m_cfgDeviceMapping.set(entryNum, mapping);
    m_cfgDeviceMappingDirty = true;
}

// ---------------------------------------------------------------------------------------------------------------------

bool HSDConfig::deleteDeviceMappingEntry(int entryNum) {
    bool removed(m_cfgDeviceMapping.removeAt(entryNum));
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

void HSDConfig::addColorMappingEntry(int entryNum, String msg, uint32_t color, Behavior behavior) {
    Serial.print(F("Adding or editing color mapping entry at index "));
    Serial.println(String(entryNum) + ", new values: name " + msg + ", color " + String(color) + ", behavior " + String(static_cast<uint8_t>(behavior)));

    ColorMapping mapping(msg, color, behavior);

    m_cfgColorMapping.set(entryNum, mapping);
    m_cfgColorMappingDirty = true;
}

// ---------------------------------------------------------------------------------------------------------------------

bool HSDConfig::deleteColorMappingEntry(int entryNum) {
    bool removed(m_cfgColorMapping.removeAt(entryNum));
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

uint8_t HSDConfig::getLedNumber(const String& deviceName) const {
    for (int i = 0; i < m_cfgDeviceMapping.size(); i++) {
        const DeviceMapping& mapping = m_cfgDeviceMapping[i];
        if (deviceName.equals(mapping.device))
            return mapping.ledNumber;
    }
    return -1;
}

// ---------------------------------------------------------------------------------------------------------------------

String HSDConfig::getDevice(int ledNumber) const {
    for (int i = 0; i < m_cfgDeviceMapping.size(); i++) {
        const DeviceMapping& mapping = m_cfgDeviceMapping[i];
        if (ledNumber == mapping.ledNumber)
            return mapping.device;
    }
    return "";
}

// ---------------------------------------------------------------------------------------------------------------------

int HSDConfig::getColorMapIndex(const String& msg) const {
    for (unsigned int i = 0; i < m_cfgColorMapping.size(); i++) {
        const ColorMapping& mapping = m_cfgColorMapping[i];
        if (msg.equals(mapping.msg))
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
