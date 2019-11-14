#include "HSDConfig.hpp"

#include <FS.h>
#include <ArduinoJson.h>

static const int MAX_SIZE_MAIN_CONFIG_FILE = 700;
static const int JSON_BUFFER_MAIN_CONFIG_FILE = 800;

static const int MAX_SIZE_COLOR_MAPPING_CONFIG_FILE = 1500;     // 1401 exactly
static const int JSON_BUFFER_COLOR_MAPPING_CONFIG_FILE = 3800;  // 3628 exactly

static const int MAX_SIZE_DEVICE_MAPPING_CONFIG_FILE = 1900;    // 1801 exactly
static const int JSON_BUFFER_DEVICE_MAPPING_CONFIG_FILE = 4000; // 3908 exactly

static const uint8_t DEFAULT_LED_BRIGHTNESS = 50;

const constexpr HSDConfig::Map HSDConfig::DefaultColor[];

HSDConfig::HSDConfig() :
    m_cfgColorMapping(MAX_COLOR_MAP_ENTRIES),
    m_cfgDeviceMapping(MAX_DEVICE_MAP_ENTRIES),
    m_colorMappingConfigFile(String("/colormapping.json")),
    m_deviceMappingConfigFile(String("/devicemapping.json")),
    m_mainConfigFile(String("/config.json"))
{
    // reset non-configurable members
    setVersion("");
    setHost("");

    // reset configurable members
    resetMainConfigData();
    resetColorMappingConfigData();
    resetDeviceMappingConfigData();
}

// ---------------------------------------------------------------------------------------------------------------------

void HSDConfig::begin(const char* version, const char* defaultIdentifier) {
    Serial.println(F(""));
    Serial.println(F("Initializing config."));

    setVersion(version);
    setHost(defaultIdentifier);

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

void HSDConfig::resetMainConfigData() {
    Serial.println(F("Deleting main config data."));

    setWifiSSID("");
    setWifiPSK("");

    setMqttServer("");
    setMqttUser("");
    setMqttPassword("");
    setMqttStatusTopic("");
#ifdef MQTT_TEST_TOPIC
    setMqttTestTopic("");
#endif // MQTT_TEST_TOPIC    
    setMqttWillTopic("");

    setNumberOfLeds(0);
    setLedDataPin(0);
    setLedBrightness(DEFAULT_LED_BRIGHTNESS);

#ifdef HSD_CLOCK_ENABLED
    setClockPinCLK(0);
    setClockPinCLK(0);
    setClockBrightness(4);
    setClockTimeZone("CET-1CEST,M3.5.0/2,M10.5.0/3");
    setClockNTPServer("pool.ntp.org");
    setClockNTPInterval(20);
#endif // HSD_CLOCK_ENABLED
#ifdef HSD_SENSOR_ENABLED
    setSensorEnabled(false);
    setSensorPin(0);
    setSensorInterval(5);
#endif // HSD_SENSOR_ENABLED
}

// ---------------------------------------------------------------------------------------------------------------------

void HSDConfig::resetColorMappingConfigData() {
    Serial.println(F("Deleting color mapping config data."));
    m_cfgColorMapping.clear();
    m_cfgColorMappingDirty = true;
}

// ---------------------------------------------------------------------------------------------------------------------

void HSDConfig::resetDeviceMappingConfigData() {
    Serial.println(F("Deleting device mapping config data."));
    m_cfgDeviceMapping.clear();
    m_cfgDeviceMappingDirty = true;
}

// ---------------------------------------------------------------------------------------------------------------------

bool HSDConfig::readMainConfigFile() {
    bool success(false);
    char fileBuffer[MAX_SIZE_MAIN_CONFIG_FILE];

    if (m_mainConfigFile.read(fileBuffer, MAX_SIZE_MAIN_CONFIG_FILE)) {
        DynamicJsonBuffer jsonBuffer(JSON_BUFFER_MAIN_CONFIG_FILE);
        JsonObject& json = jsonBuffer.parseObject(fileBuffer);
        if (json.success()) {
            Serial.println(F("Main config data successfully parsed."));
            Serial.print(F("JSON length is ")); Serial.println(json.measureLength());
            printMainConfigFile(json);
            Serial.println(F(""));

            if (json.containsKey(JSON_KEY_HOST) && json.containsKey(JSON_KEY_WIFI_SSID) &&
                json.containsKey(JSON_KEY_WIFI_PSK) && json.containsKey(JSON_KEY_MQTT_SERVER) &&
                json.containsKey(JSON_KEY_MQTT_USER) && json.containsKey(JSON_KEY_MQTT_PASSWORD) &&
                json.containsKey(JSON_KEY_MQTT_STATUS_TOPIC) && 
                json.containsKey(JSON_KEY_MQTT_WILL_TOPIC) && json.containsKey(JSON_KEY_LED_COUNT) &&
                json.containsKey(JSON_KEY_LED_PIN) && json.containsKey(JSON_KEY_CLOCK_PIN_CLK) &&
                json.containsKey(JSON_KEY_CLOCK_PIN_DIO) && json.containsKey(JSON_KEY_CLOCK_BRIGHTNESS) &&
                json.containsKey(JSON_KEY_CLOCK_TIME_ZONE) && json.containsKey(JSON_KEY_CLOCK_NTP_SERVER) &&
                json.containsKey(JSON_KEY_CLOCK_NTP_INTERVAL)) {
                Serial.println(F("Config data is complete."));

                setHost(json[JSON_KEY_HOST]);
                setWifiSSID(json[JSON_KEY_WIFI_SSID]);
                setWifiPSK(json[JSON_KEY_WIFI_PSK]);
                setMqttServer(json[JSON_KEY_MQTT_SERVER]);
                setMqttUser(json[JSON_KEY_MQTT_USER]);
                setMqttPassword(json[JSON_KEY_MQTT_PASSWORD]);
                setMqttStatusTopic(json[JSON_KEY_MQTT_STATUS_TOPIC]);
#ifdef MQTT_TEST_TOPIC
                if (json.containsKey(JSON_KEY_MQTT_TEST_TOPIC))
                    setMqttTestTopic(json[JSON_KEY_MQTT_TEST_TOPIC]);
#endif                 
                setMqttWillTopic(json[JSON_KEY_MQTT_WILL_TOPIC]);
                setNumberOfLeds(json[JSON_KEY_LED_COUNT]);
                setLedDataPin(json[JSON_KEY_LED_PIN]);
                setLedBrightness(json[JSON_KEY_LED_BRIGHTNESS]);
#ifdef HSD_CLOCK_ENABLED
                setClockPinCLK(json[JSON_KEY_CLOCK_PIN_CLK]);
                setClockPinDIO(json[JSON_KEY_CLOCK_PIN_DIO]);
                setClockBrightness(json[JSON_KEY_CLOCK_BRIGHTNESS]);
                setClockTimeZone(json[JSON_KEY_CLOCK_TIME_ZONE]);
                setClockNTPServer(json[JSON_KEY_CLOCK_NTP_SERVER]);
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
            }
        } else {
            Serial.println(F("Could not parse config data."));
        }
    } else {
        Serial.println(F("Creating default main config file."));
        resetMainConfigData();
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
    Serial.print  (F("  • mqttPassword    : ")); Serial.println((const char*)(json[JSON_KEY_MQTT_PASSWORD]));
    Serial.print  (F("  • mqttStatusTopic : ")); Serial.println((const char*)(json[JSON_KEY_MQTT_STATUS_TOPIC]));
#ifdef MQTT_TEST_TOPIC
    Serial.print  (F("  • mqttTestTopic   : ")); Serial.println((const char*)(json[JSON_KEY_MQTT_TEST_TOPIC]));
#endif // MQTT_TEST_TOPIC    
    Serial.print  (F("  • mqttWillTopic   : ")); Serial.println((const char*)(json[JSON_KEY_MQTT_WILL_TOPIC]));
    Serial.print  (F("  • ledCount        : ")); Serial.println((const char*)(json[JSON_KEY_LED_COUNT]));
    Serial.print  (F("  • ledPin          : ")); Serial.println((const char*)(json[JSON_KEY_LED_PIN]));
    Serial.print  (F("  • ledBrightness   : ")); Serial.println((const char*)(json[JSON_KEY_LED_BRIGHTNESS]));
#ifdef HSD_CLOCK_ENABLED
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
    char fileBuffer[MAX_SIZE_COLOR_MAPPING_CONFIG_FILE];
    memset(fileBuffer, 0, MAX_SIZE_COLOR_MAPPING_CONFIG_FILE);
    resetColorMappingConfigData();

    if (m_colorMappingConfigFile.read(fileBuffer, MAX_SIZE_COLOR_MAPPING_CONFIG_FILE)) {
        DynamicJsonBuffer jsonBuffer(JSON_BUFFER_COLOR_MAPPING_CONFIG_FILE);
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
        resetColorMappingConfigData();
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
    char fileBuffer[MAX_SIZE_DEVICE_MAPPING_CONFIG_FILE];
    memset(fileBuffer, 0, MAX_SIZE_DEVICE_MAPPING_CONFIG_FILE);
    resetDeviceMappingConfigData();

    if (m_deviceMappingConfigFile.read(fileBuffer, MAX_SIZE_DEVICE_MAPPING_CONFIG_FILE)) {
        DynamicJsonBuffer jsonBuffer(JSON_BUFFER_DEVICE_MAPPING_CONFIG_FILE);
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
        resetDeviceMappingConfigData();
        writeDeviceMappingConfigFile();
    }

    m_cfgDeviceMappingDirty = false;
}

// ---------------------------------------------------------------------------------------------------------------------

void HSDConfig::writeMainConfigFile() {
    DynamicJsonBuffer jsonBuffer(JSON_BUFFER_MAIN_CONFIG_FILE);
    JsonObject& json = jsonBuffer.createObject();

    json[JSON_KEY_HOST] = m_cfgHost;
    json[JSON_KEY_WIFI_SSID] = m_cfgWifiSSID;
    json[JSON_KEY_WIFI_PSK] = m_cfgWifiPSK;
    json[JSON_KEY_MQTT_SERVER] = m_cfgMqttServer;
    json[JSON_KEY_MQTT_USER] = m_cfgMqttUser;
    json[JSON_KEY_MQTT_PASSWORD] = m_cfgMqttPassword;
    json[JSON_KEY_MQTT_STATUS_TOPIC] = m_cfgMqttStatusTopic;
#ifdef MQTT_TEST_TOPIC    
    json[JSON_KEY_MQTT_TEST_TOPIC] = m_cfgMqttTestTopic;
#endif // MQTT_TEST_TOPIC    
    json[JSON_KEY_MQTT_WILL_TOPIC] = m_cfgMqttWillTopic;
    json[JSON_KEY_LED_COUNT] = m_cfgNumberOfLeds;
    json[JSON_KEY_LED_PIN] = m_cfgLedDataPin;
    json[JSON_KEY_LED_BRIGHTNESS] = m_cfgLedBrightness;
#ifdef HSD_CLOCK_ENABLED
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

    if (!m_mainConfigFile.write(&json))
        onFileWriteError();
}

// ---------------------------------------------------------------------------------------------------------------------

void HSDConfig::writeColorMappingConfigFile() {
    DynamicJsonBuffer jsonBuffer(JSON_BUFFER_COLOR_MAPPING_CONFIG_FILE);
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

    if (!m_colorMappingConfigFile.write(&json))
        onFileWriteError();
    else
        m_cfgColorMappingDirty = false;
}

// ---------------------------------------------------------------------------------------------------------------------

void HSDConfig::writeDeviceMappingConfigFile() {
    DynamicJsonBuffer jsonBuffer(JSON_BUFFER_DEVICE_MAPPING_CONFIG_FILE);
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

    if (!m_deviceMappingConfigFile.write(&json))
        onFileWriteError();
    else
        m_cfgDeviceMappingDirty = false;
}

// ---------------------------------------------------------------------------------------------------------------------

void HSDConfig::onFileWriteError() {
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

bool HSDConfig::setStringValue(char* val, int maxLen, const char* newValue) const {
    if (strncmp(val, newValue, maxLen)) {
        strncpy(val, newValue, maxLen);
        val[maxLen] = '\0';
        return true;
    } else {
        return false;
    }
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
