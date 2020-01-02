#include "HSDConfig.hpp"

#include <ArduinoJson.h>
#ifdef ARDUINO_ARCH_ESP32
#include <SPIFFS.h>
#else
#include <FS.h>
#endif

#define JSON_KEY_COLORMAPPING_MSG      "message"
#define JSON_KEY_COLORMAPPING_COLOR    "color"
#define JSON_KEY_COLORMAPPING_BEHAVIOR "behavior"
#define JSON_KEY_DEVICEMAPPING_DEVICE  "device"
#define JSON_KEY_DEVICEMAPPING_LED     "led"

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
    m_entries.push_back(new ConfigEntry(Group::Wifi, "host", "Hostname", &m_cfgHost, "[A-Za-z0-9\\-]{1,15}", "Not a valid hostname - length must between 1 and 15")); // String
    m_entries.push_back(new ConfigEntry(Group::Wifi, "SSID", "SSID", &m_cfgWifiSSID, ".{1,32}", "Length must be between 1 and 32")); // String
    m_entries.push_back(new ConfigEntry(Group::Wifi, "PSK", "Password / PSK", &m_cfgWifiPSK, ".{8,63}", "Length must be between 8 and 63", true)); // String
    m_entries.push_back(new ConfigEntry(Group::Mqtt, "server", "Server (IP or name)", &m_cfgMqttServer));  // String
    m_entries.push_back(new ConfigEntry(Group::Mqtt, "port", "Port (1-65535)", &m_cfgMqttPort, "^([0-9]{1,4}|[1-5][0-9]{4}|6[0-4][0-9]{3}|65[0-4][0-9]{2}|655[0-2][0-9]|6553[0-5])$", "Not a valid port")); // Word
    m_entries.push_back(new ConfigEntry(Group::Mqtt, "user", "User name", &m_cfgMqttUser)); // String 
    m_entries.push_back(new ConfigEntry(Group::Mqtt, "password", "Password", &m_cfgMqttPassword, "", "", true)); // String
    m_entries.push_back(new ConfigEntry(Group::Mqtt, "statusTopic", "Status topic", &m_cfgMqttStatusTopic)); // String
#ifdef MQTT_TEST_TOPIC
    m_entries.push_back(new ConfigEntry(Group::Mqtt, "testTopic", "Test topic", &m_cfgMqttTestTopic)); // String
#endif // MQTT_TEST_TOPIC
    m_entries.push_back(new ConfigEntry(Group::Mqtt, "outTopic", "Outgoing topic", &m_cfgMqttOutTopic)); // String
    m_entries.push_back(new ConfigEntry(Group::Leds, "count", "Number of LEDs", &m_cfgNumberOfLeds, 255)); // Slider
    m_entries.push_back(new ConfigEntry(Group::Leds, "pin", "LED pin", &m_cfgLedDataPin)); // Gpio
    m_entries.push_back(new ConfigEntry(Group::Leds, "brightness", "Brightness", &m_cfgLedBrightness, 255)); // Slider
    m_entries.push_back(new ConfigEntry(Group::Leds, "colorMapping", &m_cfgColorMapping)); // ColorMapping
    m_entries.push_back(new ConfigEntry(Group::Leds, "deviceMapping", &m_cfgDeviceMapping)); // DeviceMapping
#ifdef HSD_CLOCK_ENABLED
    m_entries.push_back(new ConfigEntry(Group::Clock, "enabled", "Enabled", &m_cfgClockEnabled)); // Bool
    m_entries.push_back(new ConfigEntry(Group::Clock, "CLK", "CLK pin", &m_cfgClockPinCLK)); // Gpio
    m_entries.push_back(new ConfigEntry(Group::Clock, "DIO", "DIO pin", &m_cfgClockPinDIO)); // Gpio
    m_entries.push_back(new ConfigEntry(Group::Clock, "brightness", "Brightness", &m_cfgClockBrightness, 8)); // Slider
    m_entries.push_back(new ConfigEntry(Group::Clock, "timezone", "Time zone (e.g. CET-1CEST,M3.5.0/2,M10.5.0/3)", &m_cfgClockTimeZone)); // String
    m_entries.push_back(new ConfigEntry(Group::Clock, "server", "NTP server (e.g. pool.ntp.org)", &m_cfgClockNTPServer)); // String
    m_entries.push_back(new ConfigEntry(Group::Clock, "interval", "NTP update interval (min.)", &m_cfgClockNTPInterval, 255)); // Slider
#endif // HSD_CLOCK_ENABLED
#ifdef HSD_SENSOR_ENABLED
    m_entries.push_back(new ConfigEntry(Group::Sensors, "sonoffEnabled", "Sonoff SI7021", &m_cfgSensorSonoffEnabled)); // Bool
    m_entries.push_back(new ConfigEntry(Group::Sensors, "sonoffPin", "Data pin", &m_cfgSensorPin)); // Gpio
    m_entries.push_back(new ConfigEntry(Group::Sensors, "interval", "Sensor update interval (min.)", &m_cfgSensorInterval, 60)); // Slider
    m_entries.push_back(new ConfigEntry(Group::Sensors, "i2cEnabled", "I2C", &m_cfgSensorI2CEnabled)); // Bool - DONE
    m_entries.push_back(new ConfigEntry(Group::Sensors, "altitude", "Altitude", &m_cfgSensorAltitude, "[0-9]{1,4}", "0")); // Word -> InputField
#endif // HSD_SENSOR_ENABLED
#if defined HSD_BLUETOOTH_ENABLED && defined ARDUINO_ARCH_ESP32
    m_entries.push_back(new ConfigEntry(Group::Bluetooth, "enabled", "Enabled", &m_cfgBluetoothEnabled)); // Bool
#endif // HSD_BLUETOOTH_ENABLED
}

// ---------------------------------------------------------------------------------------------------------------------

void HSDConfig::begin() {
    Serial.printf("\nInitializing config (%u entries) - using ArduinoJson version %s\n", m_entries.size(), ARDUINOJSON_VERSION);
    if (SPIFFS.begin()) {
        Serial.println("Mounted file system.");
        readConfigFile();
    } else {
        Serial.println("Failed to mount file system");
    }
}

// ---------------------------------------------------------------------------------------------------------------------

bool HSDConfig::readConfigFile() {
    bool success(false);
    String fileBuffer;

    Serial.printf("Reading config file %s: ", FILENAME_MAINCONFIG);
    if (SPIFFS.exists(FILENAME_MAINCONFIG)) {
        File configFile = SPIFFS.open(FILENAME_MAINCONFIG, "r");
        if (configFile) {
            size_t size = configFile.size();
            Serial.printf("file size is %u bytes\n", size);
            char* buffer = new char[size + 1];
            buffer[size] = 0;
            configFile.readBytes(buffer, size);
            fileBuffer = buffer;
            delete[] buffer;
            configFile.close();
            
            DynamicJsonBuffer jsonBuffer(fileBuffer.length() + 1);
            JsonObject& root = jsonBuffer.parseObject(fileBuffer);
            if (root.success()) {
                Serial.println("Config data successfully parsed.");
                int maxLen(0), len(0);
                for (size_t idx = 0; idx < m_entries.size(); idx++) {
                    len = m_entries[idx]->key.length() + groupDescription(m_entries[idx]->group).length() + 1;
                    if (len > maxLen)
                        maxLen = len;
                }

                Group prevGroup = Group::__Last;
                JsonObject* json = nullptr;
                String groupName;
                for (size_t idx = 0; idx < m_entries.size(); idx++) {
                    const ConfigEntry* entry = m_entries[idx];
                    if (prevGroup != entry->group) {
                        prevGroup = entry->group;
                        groupName = groupDescription(prevGroup);
                        groupName.toLowerCase();
                        if (root.containsKey(groupName))
                            json = &root[groupName].as<JsonObject>();
                        else
                            json = nullptr;
                    }

                    Serial.printf("  • %s.%s", groupName.c_str(), entry->key.c_str());
                    for (int i = groupName.length() + 1 + entry->key.length(); i < maxLen; i++)
                        Serial.print(" ");
                    Serial.print(": ");
                    if (entry->type == DataType::Password)
                        Serial.println("not shown");
                    else if (json)
                        Serial.println((*json)[entry->key].as<String>());
                    else
                        Serial.println("");
                    if (json && json->containsKey(entry->key)) {
                        switch (entry->type) {
                            case DataType::Password:
                            case DataType::String: *entry->value.string  = (*json)[entry->key].as<String>(); break;
                            case DataType::Bool:   *entry->value.boolean = (*json)[entry->key].as<bool>();   break;
                            case DataType::Gpio:
                            case DataType::Slider: *entry->value.byte    = (*json)[entry->key].as<int>();    break;
                            case DataType::Word:   *entry->value.word    = (*json)[entry->key].as<int>();    break;
                            case DataType::ColorMapping: {
                                entry->value.colMap->clear();
                                const JsonArray& colMap = (*json)[entry->key].as<JsonArray>();
                                for (size_t i = 0; i < colMap.size(); i++) {
                                    const JsonObject& elem = colMap.get<JsonVariant>(i).as<JsonObject>();
                                    if (elem.containsKey(JSON_KEY_COLORMAPPING_MSG) && elem.containsKey(JSON_KEY_COLORMAPPING_COLOR) &&
                                        elem.containsKey(JSON_KEY_COLORMAPPING_BEHAVIOR))
                                        entry->value.colMap->push_back(new ColorMapping(elem[JSON_KEY_COLORMAPPING_MSG].as<String>(), 
                                                                                        elem[JSON_KEY_COLORMAPPING_COLOR].as<uint32_t>(), 
                                                                                        static_cast<Behavior>(elem[JSON_KEY_COLORMAPPING_BEHAVIOR].as<int>())));
                                }
                                break;
                            }         
                            case DataType::DeviceMapping: {
                                entry->value.devMap->clear();
                                const JsonArray& devMap = (*json)[entry->key].as<JsonArray>();
                                for (size_t i = 0; i < devMap.size(); i++) {
                                    const JsonObject& elem = devMap.get<JsonVariant>(i).as<JsonObject>();
                                    if (elem.containsKey(JSON_KEY_DEVICEMAPPING_DEVICE) && elem.containsKey(JSON_KEY_DEVICEMAPPING_LED))
                                        entry->value.devMap->push_back(new DeviceMapping(elem[JSON_KEY_DEVICEMAPPING_DEVICE].as<String>(),
                                                                                         elem[JSON_KEY_DEVICEMAPPING_LED].as<int>()));
                                }
                                break;
                            }
                        }
                    }
                } 
                success = true;
            } else {
                Serial.println("Could not parse config data.");
            }
        } else {
            Serial.println("file open failed");
        }
    } else {
        Serial.println("File does not exist");
    }
    if (!success) {
        Serial.println("File does not exist - creating default main config file.");
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
    for (size_t idx = 0; idx < m_entries.size(); idx++) {
        const ConfigEntry* entry = m_entries[idx];
        if (prevGroup != entry->group) {
            prevGroup = entry->group;
            String groupName = groupDescription(prevGroup);
            groupName.toLowerCase();
            json = &root.createNestedObject(groupName);
        }
        switch (entry->type) {
            case DataType::Password:
            case DataType::String: (*json)[entry->key] = *entry->value.string;  break;
            case DataType::Bool:   (*json)[entry->key] = *entry->value.boolean; break;
            case DataType::Gpio:
            case DataType::Slider: (*json)[entry->key] = *entry->value.byte;    break;
            case DataType::Word:   (*json)[entry->key] = *entry->value.word;    break;
            case DataType::ColorMapping: {
                JsonArray& colMapping = json->createNestedArray(entry->key);
                for (unsigned int index = 0; index < entry->value.colMap->size(); index++) {
                    JsonObject& colorMappingEntry = colMapping.createNestedObject(); 
                    auto mapping = entry->value.colMap->at(index);
                    colorMappingEntry[JSON_KEY_COLORMAPPING_MSG] = mapping->msg;
                    colorMappingEntry[JSON_KEY_COLORMAPPING_COLOR] = mapping->color;
                    colorMappingEntry[JSON_KEY_COLORMAPPING_BEHAVIOR] = static_cast<int>(mapping->behavior);
                }
                break;
            }
            case DataType::DeviceMapping: {
                JsonArray& devMapping = json->createNestedArray(entry->key);
                for (unsigned int index = 0; index < entry->value.devMap->size(); index++) {
                    JsonObject& deviceMappingEntry = devMapping.createNestedObject(); 
                    auto mapping = entry->value.devMap->at(index);
                    deviceMappingEntry[JSON_KEY_DEVICEMAPPING_DEVICE] = mapping->device;
                    deviceMappingEntry[JSON_KEY_DEVICEMAPPING_LED]  = static_cast<int>(mapping->ledNumber);
                }
                break;
            }
        }
    }
    Serial.printf("Writing config file %s\n", FILENAME_MAINCONFIG);
    File configFile = SPIFFS.open(FILENAME_MAINCONFIG, "w+");
    if (configFile) {
        root.prettyPrintTo(configFile);
        configFile.close();
    } else {
        Serial.println("Failed to write file, formatting file system.");
        SPIFFS.format();
        Serial.println("Done.");
    }
}

// ---------------------------------------------------------------------------------------------------------------------

uint8_t HSDConfig::getLedNumber(const String& deviceName) const {
    for (int i = 0; i < m_cfgDeviceMapping.size(); i++) {
        auto mapping = m_cfgDeviceMapping.at(i);
        if (deviceName.equals(mapping->device))
            return mapping->ledNumber;
    }
    return -1;
}

// ---------------------------------------------------------------------------------------------------------------------

String HSDConfig::getDevice(int ledNumber) const {
    for (int i = 0; i < m_cfgDeviceMapping.size(); i++) {
        auto mapping = m_cfgDeviceMapping.at(i);
        if (ledNumber == mapping->ledNumber)
            return mapping->device;
    }
    return "";
}

// ---------------------------------------------------------------------------------------------------------------------

int HSDConfig::getColorMapIndex(const String& msg) const {
    for (unsigned int i = 0; i < m_cfgColorMapping.size(); i++) {
        auto mapping = m_cfgColorMapping.at(i);
        if (msg.equals(mapping->msg))
            return i;
    }
    return -1;
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
        case Group::Wifi:      return "WiFi";
        case Group::Mqtt:      return "MQTT";
        case Group::Leds:      return "LEDs";
        case Group::Clock:     return "Clock";
        case Group::Sensors:   return "Sensors";
        case Group::Bluetooth: return "Bluetooth";
        default:
            Serial.printf("groupDescription: Group %u UNKNOWN\n", static_cast<uint8_t>(group));
            return "";
    }
}

// ---------------------------------------------------------------------------------------------------------------------

void HSDConfig::setColorMap(vector<ColorMapping*>& values) {
    ColorMapping* elem = nullptr;
    while ((elem = m_cfgColorMapping.back())) {
        delete elem;
        m_cfgColorMapping.pop_back();
    }
    m_cfgColorMapping.assign(values.begin(), values.end());
}

// ---------------------------------------------------------------------------------------------------------------------

void HSDConfig::setDeviceMap(vector<DeviceMapping*>& values) {
    DeviceMapping* elem = nullptr;
    while ((elem = m_cfgDeviceMapping.back())) {
        delete elem;
        m_cfgDeviceMapping.pop_back();
    }
    m_cfgDeviceMapping.assign(values.begin(), values.end());
}
