#include "HSDConfig.hpp"

#include <ArduinoJson.h>
#ifdef ARDUINO_ARCH_ESP32
#include <SPIFFS.h>
#else
#include <FS.h>
#endif

#define JSON_KEY_COLORMAPPING_MSG      (F("message"))
#define JSON_KEY_COLORMAPPING_COLOR    (F("color"))
#define JSON_KEY_COLORMAPPING_BEHAVIOR (F("behavior"))
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
    m_cfgEntries.push_back(ConfigEntry(Group::Wifi, F("host"), F("Hostname"), F("host"), &m_cfgHost));
    m_cfgEntries.push_back(ConfigEntry(Group::Wifi, F("SSID"), F("SSID"), F("SSID"), &m_cfgWifiSSID));
    m_cfgEntries.push_back(ConfigEntry(Group::Wifi, F("PSK"), F("Password"), F("Password"), &m_cfgWifiPSK, true));
    m_cfgEntries.push_back(ConfigEntry(Group::Mqtt, F("server"), F("Server"), F("IP or hostname"), &m_cfgMqttServer));
    m_cfgEntries.push_back(ConfigEntry(Group::Mqtt, F("port"), F("Port"), F("Port"), &m_cfgMqttPort));
    m_cfgEntries.push_back(ConfigEntry(Group::Mqtt, F("user"), F("User"), F("User name"), &m_cfgMqttUser));
    m_cfgEntries.push_back(ConfigEntry(Group::Mqtt, F("password"), F("Password"), F("Password"), &m_cfgMqttPassword, true));
    m_cfgEntries.push_back(ConfigEntry(Group::Mqtt, F("statusTopic"), F("Status topic"), F("#"), &m_cfgMqttStatusTopic));
#ifdef MQTT_TEST_TOPIC
    m_cfgEntries.push_back(ConfigEntry(Group::Mqtt, F("testTopic"), F("Test topic"), F("#"), &m_cfgMqttTestTopic));
#endif // MQTT_TEST_TOPIC
    m_cfgEntries.push_back(ConfigEntry(Group::Mqtt, F("outTopic"), F("Outgoing topic"), F("#"), &m_cfgMqttOutTopic));
    m_cfgEntries.push_back(ConfigEntry(Group::Leds, F("count"), F("Number of LEDs"), F("0"), 3, &m_cfgNumberOfLeds));
    m_cfgEntries.push_back(ConfigEntry(Group::Leds, F("pin"), F("LED pin"), F("0"), 2, &m_cfgLedDataPin));
    m_cfgEntries.push_back(ConfigEntry(Group::Leds, F("brightness"), F("Brightness"), F("0-255"), 3, &m_cfgLedBrightness));
    m_cfgEntries.push_back(ConfigEntry(Group::Leds, F("colorMapping"), &m_cfgColorMapping));
    m_cfgEntries.push_back(ConfigEntry(Group::Leds, F("deviceMapping"), &m_cfgDeviceMapping));
#ifdef HSD_CLOCK_ENABLED
    m_cfgEntries.push_back(ConfigEntry(Group::Clock, F("enabled"), F("Enable"), &m_cfgClockEnabled));
    m_cfgEntries.push_back(ConfigEntry(Group::Clock, F("CLK"), F("CLK pin"), F("0"), 2, &m_cfgClockPinCLK));
    m_cfgEntries.push_back(ConfigEntry(Group::Clock, F("DIO"), F("DIO pin"), F("0"), 2, &m_cfgClockPinDIO));
    m_cfgEntries.push_back(ConfigEntry(Group::Clock, F("brightness"), F("Brightness"), F("0-8"), 1, &m_cfgClockBrightness));
    m_cfgEntries.push_back(ConfigEntry(Group::Clock, F("timezone"), F("Time zone"), F("CET-1CEST,M3.5.0/2,M10.5.0/3"), &m_cfgClockTimeZone));
    m_cfgEntries.push_back(ConfigEntry(Group::Clock, F("server"), F("NTP server"), F("pool.ntp.org"), &m_cfgClockNTPServer));
    m_cfgEntries.push_back(ConfigEntry(Group::Clock, F("interval"), F("NTP update interval (min.)"), F("20"), &m_cfgClockNTPInterval));
#endif // HSD_CLOCK_ENABLED
#ifdef HSD_SENSOR_ENABLED
    m_cfgEntries.push_back(ConfigEntry(Group::Sensors, F("sonoffEnabled"), F("Sonoff SI7021"), &m_cfgSensorSonoffEnabled));
    m_cfgEntries.push_back(ConfigEntry(Group::Sensors, F("sonoffPin"), F("Data pin"), F("0"), 2, &m_cfgSensorPin));
    m_cfgEntries.push_back(ConfigEntry(Group::Sensors, F("interval"), F("Sensor update interval (min.)"), F("5"), &m_cfgSensorInterval));
    m_cfgEntries.push_back(ConfigEntry(Group::Sensors, F("i2cEnabled"), F("I2C"), &m_cfgSensorI2CEnabled));
    m_cfgEntries.push_back(ConfigEntry(Group::Sensors, F("altitude"), F("Altitude"), F("0"), &m_cfgSensorAltitude));
#endif // HSD_SENSOR_ENABLED
#if defined HSD_BLUETOOTH_ENABLED && defined ARDUINO_ARCH_ESP32
    m_cfgEntries.push_back(ConfigEntry(Group::Bluetooth, F("enabled"), F("Enable"), &m_cfgBluetoothEnabled));
#endif // HSD_BLUETOOTH_ENABLED
}

// ---------------------------------------------------------------------------------------------------------------------

void HSDConfig::begin() {
    Serial.printf("\r\nInitializing config (%u entries) - using ArduinoJson version %s\r\n", m_cfgEntries.size(), ARDUINOJSON_VERSION);
    if (SPIFFS.begin()) {
        Serial.println(F("Mounted file system."));
        readConfigFile();
    } else {
        Serial.println(F("Failed to mount file system"));
    }
}

// ---------------------------------------------------------------------------------------------------------------------

bool HSDConfig::readConfigFile() {
    bool success(false);
    String fileBuffer;

    Serial.print(F("Reading config file ")); Serial.print(FILENAME_MAINCONFIG); Serial.print(F(": "));
    if (SPIFFS.exists(FILENAME_MAINCONFIG)) {
        File configFile = SPIFFS.open(FILENAME_MAINCONFIG, "r");
        if (configFile) {
            size_t size = configFile.size();
            Serial.printf("file size is %u bytes\r\n", size);
            char* buffer = new char[size + 1];
            buffer[size] = 0;
            configFile.readBytes(buffer, size);
            fileBuffer = buffer;
            delete[] buffer;
            configFile.close();
            
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
                    const ConfigEntry& entry = m_cfgEntries[idx];
                    if (prevGroup != entry.group) {
                        prevGroup = entry.group;
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
                    Serial.print(entry.key);
                    for (int i = groupName.length() + 1 + strlen_P((PGM_P)entry.key); i < maxLen; i++)
                        Serial.print(F(" "));
                    Serial.print(F(": "));
                    if (entry.type == DataType::Password)
                        Serial.println("not shown");
                    else if (json)
                        Serial.println((*json)[entry.key].as<String>());
                    else
                        Serial.println(F(""));
                    if (json && json->containsKey(entry.key)) {
                        switch (entry.type) {
                            case DataType::Password:
                            case DataType::String: *entry.value.string  = (*json)[entry.key].as<String>(); break;
                            case DataType::Bool:   *entry.value.boolean = (*json)[entry.key].as<bool>();   break;
                            case DataType::Byte:   *entry.value.byte    = (*json)[entry.key].as<int>();    break;
                            case DataType::Word:   *entry.value.word    = (*json)[entry.key].as<int>();    break;
                            case DataType::ColorMapping: {
                                entry.value.colMap->clear();
                                const JsonArray& colMap = (*json)[entry.key].as<JsonArray>();
                                for (size_t i = 0; i < colMap.size(); i++) {
                                    const JsonObject& elem = colMap.get<JsonVariant>(i).as<JsonObject>();
                                    if (elem.containsKey(JSON_KEY_COLORMAPPING_MSG) && elem.containsKey(JSON_KEY_COLORMAPPING_COLOR) &&
                                        elem.containsKey(JSON_KEY_COLORMAPPING_BEHAVIOR))
                                        entry.value.colMap->push_back(ColorMapping(elem[JSON_KEY_COLORMAPPING_MSG].as<String>(), 
                                                                                   elem[JSON_KEY_COLORMAPPING_COLOR].as<uint32_t>(), 
                                                                                   static_cast<Behavior>(elem[JSON_KEY_COLORMAPPING_BEHAVIOR].as<int>())));
                                }
                                break;
                            }         
                            case DataType::DeviceMapping: {
                                entry.value.devMap->clear();
                                const JsonArray& devMap = (*json)[entry.key].as<JsonArray>();
                                for (size_t i = 0; i < devMap.size(); i++) {
                                    const JsonObject& elem = devMap.get<JsonVariant>(i).as<JsonObject>();
                                    if (elem.containsKey(JSON_KEY_DEVICEMAPPING_DEVICE) && elem.containsKey(JSON_KEY_DEVICEMAPPING_LED))
                                        entry.value.devMap->push_back(DeviceMapping(elem[JSON_KEY_DEVICEMAPPING_DEVICE].as<String>(),
                                                                                    elem[JSON_KEY_DEVICEMAPPING_LED].as<int>()));
                                }
                                break;
                            }
                        }
                    }
                } 
                success = true;
            } else {
                Serial.println(F("Could not parse config data."));
            }
        } else {
            Serial.println(F("file open failed"));
        }
    } else {
        Serial.println(F("File does not exist"));
    }
    if (!success) {
        Serial.println(F("File does not exist - creating default main config file."));
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
        const ConfigEntry& entry = m_cfgEntries[idx];
        if (prevGroup != entry.group) {
            prevGroup = entry.group;
            String groupName = groupDescription(prevGroup);
            groupName.toLowerCase();
            json = &root.createNestedObject(groupName);
        }
        switch (entry.type) {
            case DataType::Password:
            case DataType::String: (*json)[entry.key] = *entry.value.string;  break;
            case DataType::Bool:   (*json)[entry.key] = *entry.value.boolean; break;
            case DataType::Byte:   (*json)[entry.key] = *entry.value.byte;    break;
            case DataType::Word:   (*json)[entry.key] = *entry.value.word;    break;
            case DataType::ColorMapping: {
                JsonArray& colMapping = json->createNestedArray(entry.key);
                for (unsigned int index = 0; index < entry.value.colMap->size(); index++) {
                    JsonObject& colorMappingEntry = colMapping.createNestedObject(); 
                    const ColorMapping& mapping = (*entry.value.colMap)[index];
                    colorMappingEntry[JSON_KEY_COLORMAPPING_MSG] = mapping.msg;
                    colorMappingEntry[JSON_KEY_COLORMAPPING_COLOR] = mapping.color;
                    colorMappingEntry[JSON_KEY_COLORMAPPING_BEHAVIOR] = static_cast<int>(mapping.behavior);
                }
                break;
            }
            case DataType::DeviceMapping: {
                JsonArray& devMapping = json->createNestedArray(entry.key);
                for (unsigned int index = 0; index < entry.value.devMap->size(); index++) {
                    JsonObject& deviceMappingEntry = devMapping.createNestedObject(); 
                    const DeviceMapping& mapping = (*entry.value.devMap)[index];
                    deviceMappingEntry[JSON_KEY_DEVICEMAPPING_DEVICE] = mapping.device;
                    deviceMappingEntry[JSON_KEY_DEVICEMAPPING_LED]  = static_cast<int>(mapping.ledNumber);
                }
                break;
            }
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

// ---------------------------------------------------------------------------------------------------------------------

void HSDConfig::setColorMap(QList<ColorMapping>& values) {
    m_cfgColorMapping.clear();
    while (values.size() > 0) {
        ColorMapping& val = values.front();
        m_cfgColorMapping.push_back(val);
        values.pop_front();
    }
}

// ---------------------------------------------------------------------------------------------------------------------

void HSDConfig::setDeviceMap(QList<DeviceMapping>& values) {
    m_cfgDeviceMapping.clear();
    while (values.size() > 0) {
        DeviceMapping& val = values.front();
        m_cfgDeviceMapping.push_back(val);
        values.pop_front();
    }
}
