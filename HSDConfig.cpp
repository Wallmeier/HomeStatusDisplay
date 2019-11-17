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

HSDConfig::HSDConfig(const char* version, const char* defaultIdentifier) :
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
    m_cfgHost(defaultIdentifier),
    m_cfgLedBrightness(50),
    m_cfgLedDataPin(0),
    m_cfgMqttPort(1833),
    m_cfgNumberOfLeds(0),
#ifdef HSD_SENSOR_ENABLED
    m_cfgSensorEnabled(false),
    m_cfgSensorInterval(2),
    m_cfgSensorPin(0),
#endif // HSD_SENSOR_ENABLED
    m_cfgVersion(version)
{
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

            if (json.containsKey(JSON_KEY_HOST))
                setHost(json[JSON_KEY_HOST]);
            if (json.containsKey(JSON_KEY_WIFI_SSID))
                setWifiSSID(json[JSON_KEY_WIFI_SSID]);
            if (json.containsKey(JSON_KEY_WIFI_PSK))
                setWifiPSK(json[JSON_KEY_WIFI_PSK]);
            if (json.containsKey(JSON_KEY_MQTT_SERVER))
                setMqttServer(json[JSON_KEY_MQTT_SERVER]);
            if (json.containsKey(JSON_KEY_MQTT_USER))
                setMqttUser(json[JSON_KEY_MQTT_USER]);
            if (json.containsKey(JSON_KEY_MQTT_PASSWORD))
                setMqttPassword(json[JSON_KEY_MQTT_PASSWORD]);
            if (json.containsKey(JSON_KEY_MQTT_PORT))
                setMqttPort(json[JSON_KEY_MQTT_PORT]);
            if (json.containsKey(JSON_KEY_MQTT_STATUS_TOPIC))
                setMqttStatusTopic(json[JSON_KEY_MQTT_STATUS_TOPIC]);
#ifdef MQTT_TEST_TOPIC
            if (json.containsKey(JSON_KEY_MQTT_TEST_TOPIC))
                setMqttTestTopic(json[JSON_KEY_MQTT_TEST_TOPIC]);
#endif          
            if (json.containsKey(JSON_KEY_MQTT_OUT_TOPIC))
                setMqttOutTopic(json[JSON_KEY_MQTT_OUT_TOPIC]);
            if (json.containsKey(JSON_KEY_LED_COUNT))
                setNumberOfLeds(json[JSON_KEY_LED_COUNT]);
            if (json.containsKey(JSON_KEY_LED_PIN))
                setLedDataPin(json[JSON_KEY_LED_PIN]);
            if (json.containsKey(JSON_KEY_LED_BRIGHTNESS))
                setLedBrightness(json[JSON_KEY_LED_BRIGHTNESS]);
#ifdef HSD_CLOCK_ENABLED
            if (json.containsKey(JSON_KEY_CLOCK_ENABLED))
                setClockEnabled(json[JSON_KEY_CLOCK_ENABLED]);
            if (json.containsKey(JSON_KEY_CLOCK_PIN_CLK))
                setClockPinCLK(json[JSON_KEY_CLOCK_PIN_CLK]);
            if (json.containsKey(JSON_KEY_CLOCK_PIN_DIO))
                setClockPinDIO(json[JSON_KEY_CLOCK_PIN_DIO]);
            if (json.containsKey(JSON_KEY_CLOCK_BRIGHTNESS))
                setClockBrightness(json[JSON_KEY_CLOCK_BRIGHTNESS]);
            if (json.containsKey(JSON_KEY_CLOCK_TIME_ZONE))
                setClockTimeZone(json[JSON_KEY_CLOCK_TIME_ZONE]);
            if (json.containsKey(JSON_KEY_CLOCK_NTP_SERVER))
                setClockNTPServer(json[JSON_KEY_CLOCK_NTP_SERVER]);
            if (json.containsKey(JSON_KEY_CLOCK_NTP_INTERVAL))
                setClockNTPInterval(json[JSON_KEY_CLOCK_NTP_INTERVAL]);
#endif // HSD_CLOCK_ENABLED
#ifdef HSD_SENSOR_ENABLED
            if (json.containsKey(JSON_KEY_SENSOR_ENABLED))
                setSensorEnabled(json[JSON_KEY_SENSOR_ENABLED]);
            if (json.containsKey(JSON_KEY_SENSOR_PIN))
                setSensorPin(json[JSON_KEY_SENSOR_PIN]);
            if (json.containsKey(JSON_KEY_SENSOR_INTERVAL))
                setSensorInterval(json[JSON_KEY_SENSOR_INTERVAL]);
#endif // HSD_SENSOR_ENABLED
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
    Serial.print  (F("  • host            : ")); Serial.println((const char*)(json[JSON_KEY_HOST]));
    Serial.print  (F("  • wifiSSID        : ")); Serial.println((const char*)(json[JSON_KEY_WIFI_SSID]));
    Serial.println(F("  • wifiPSK         : not shown"));
    Serial.print  (F("  • mqttServer      : ")); Serial.println((const char*)(json[JSON_KEY_MQTT_SERVER]));
    Serial.print  (F("  • mqttUser        : ")); Serial.println((const char*)(json[JSON_KEY_MQTT_USER]));
    Serial.println(F("  • mqttPassword    : not shown"));
    Serial.print  (F("  • mqttPort        : ")); Serial.println((const char*)(json[JSON_KEY_MQTT_PORT]));
    Serial.print  (F("  • mqttStatusTopic : ")); Serial.println((const char*)(json[JSON_KEY_MQTT_STATUS_TOPIC]));
#ifdef MQTT_TEST_TOPIC
    Serial.print  (F("  • mqttTestTopic   : ")); Serial.println((const char*)(json[JSON_KEY_MQTT_TEST_TOPIC]));
#endif // MQTT_TEST_TOPIC    
    Serial.print  (F("  • mqttOutTopic    : ")); Serial.println((const char*)(json[JSON_KEY_MQTT_OUT_TOPIC]));
    Serial.print  (F("  • ledCount        : ")); Serial.println((const char*)(json[JSON_KEY_LED_COUNT]));
    Serial.print  (F("  • ledPin          : ")); Serial.println((const char*)(json[JSON_KEY_LED_PIN]));
    Serial.print  (F("  • ledBrightness   : ")); Serial.println((const char*)(json[JSON_KEY_LED_BRIGHTNESS]));
#ifdef HSD_CLOCK_ENABLED
    Serial.print  (F("  • clockEnabled    : ")); Serial.println((const char*)(json[JSON_KEY_CLOCK_ENABLED]));
    Serial.print  (F("  • clockPinCLK     : ")); Serial.println((const char*)(json[JSON_KEY_CLOCK_PIN_CLK]));
    Serial.print  (F("  • clockPinDIO     : ")); Serial.println((const char*)(json[JSON_KEY_CLOCK_PIN_DIO]));
    Serial.print  (F("  • clockBrightness : ")); Serial.println((const char*)(json[JSON_KEY_CLOCK_BRIGHTNESS]));
    Serial.print  (F("  • clockTimeZone   : ")); Serial.println((const char*)(json[JSON_KEY_CLOCK_TIME_ZONE]));
    Serial.print  (F("  • clockNTPServer  : ")); Serial.println((const char*)(json[JSON_KEY_CLOCK_NTP_SERVER]));
    Serial.print  (F("  • clockNTPInterval: ")); Serial.println((const char*)(json[JSON_KEY_CLOCK_NTP_INTERVAL]));
#endif // HSD_CLOCK_ENABLED
#ifdef HSD_SENSOR_ENABLED
    Serial.print  (F("  • sensorEnabled   : ")); Serial.println((const char*)(json[JSON_KEY_SENSOR_ENABLED]));
    Serial.print  (F("  • sensorPin       : ")); Serial.println((const char*)(json[JSON_KEY_SENSOR_PIN]));
    Serial.print  (F("  • sensorInterval  : ")); Serial.println((const char*)(json[JSON_KEY_SENSOR_INTERVAL]));
#endif // HSD_SENSOR_ENABLED
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
            addColorMappingEntry(i - 1, temp, DefaultColor[i].value, ON);
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

    json[JSON_KEY_HOST] = m_cfgHost;
    json[JSON_KEY_WIFI_SSID] = m_cfgWifiSSID;
    json[JSON_KEY_WIFI_PSK] = m_cfgWifiPSK;
    json[JSON_KEY_MQTT_SERVER] = m_cfgMqttServer;
    json[JSON_KEY_MQTT_USER] = m_cfgMqttUser;
    json[JSON_KEY_MQTT_PASSWORD] = m_cfgMqttPassword;
    json[JSON_KEY_MQTT_PORT] = m_cfgMqttPort;
    json[JSON_KEY_MQTT_STATUS_TOPIC] = m_cfgMqttStatusTopic;
#ifdef MQTT_TEST_TOPIC    
    json[JSON_KEY_MQTT_TEST_TOPIC] = m_cfgMqttTestTopic;
#endif // MQTT_TEST_TOPIC    
    json[JSON_KEY_MQTT_OUT_TOPIC] = m_cfgMqttOutTopic;
    json[JSON_KEY_LED_COUNT] = m_cfgNumberOfLeds;
    json[JSON_KEY_LED_PIN] = m_cfgLedDataPin;
    json[JSON_KEY_LED_BRIGHTNESS] = m_cfgLedBrightness;
#ifdef HSD_CLOCK_ENABLED
    json[JSON_KEY_CLOCK_ENABLED] = m_cfgClockEnabled;
    json[JSON_KEY_CLOCK_PIN_CLK] = m_cfgClockPinCLK;
    json[JSON_KEY_CLOCK_PIN_DIO] = m_cfgClockPinDIO;
    json[JSON_KEY_CLOCK_BRIGHTNESS] = m_cfgClockBrightness;
    json[JSON_KEY_CLOCK_TIME_ZONE] = m_cfgClockTimeZone;
    json[JSON_KEY_CLOCK_NTP_SERVER] = m_cfgClockNTPServer;
    json[JSON_KEY_CLOCK_NTP_INTERVAL] = m_cfgClockNTPInterval;
#endif // HSD_CLOCK_ENABLED
#ifdef HSD_SENSOR_ENABLED
    json[JSON_KEY_SENSOR_ENABLED] = m_cfgSensorEnabled;
    json[JSON_KEY_SENSOR_PIN] = m_cfgSensorPin;
    json[JSON_KEY_SENSOR_INTERVAL] = m_cfgSensorInterval;
#endif

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
    Serial.println(String(entryNum) + ", new values: name " + msg + ", color " + String(color) + ", behavior " + String(behavior));

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