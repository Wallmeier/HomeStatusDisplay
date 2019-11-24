#ifndef HSDCONFIG_H
#define HSDCONFIG_H

#include <Arduino.h>
#include <ArduinoJson.h>

#include "PreAllocatedLinkedList.hpp"

#define HSD_VERSION (F("0.9"))

// comment out next line if you do not need the clock module
#define HSD_CLOCK_ENABLED
// comment out next line if you do not need the sensor module (Sonoff SI7021)
#define HSD_SENSOR_ENABLED
// #define MQTT_TEST_TOPIC
#define HSD_BLUETOOTH_ENABLED

#define JSON_KEY_COLORMAPPING_MSG      (F("m"))
#define JSON_KEY_COLORMAPPING_COLOR    (F("c"))
#define JSON_KEY_COLORMAPPING_BEHAVIOR (F("b"))
#define JSON_KEY_DEVICEMAPPING_NAME    (F("n"))
#define JSON_KEY_DEVICEMAPPING_LED     (F("l"))

#define MAX_COLOR_MAP_ENTRIES          30
#define MAX_COLOR_MAPPING_MSG_LEN      15
#define MAX_DEVICE_MAP_ENTRIES         35
#define MAX_DEVICE_MAPPING_NAME_LEN    25
#define NUMBER_OF_DEFAULT_COLORS       9

class HSDConfig {
public:
    /*
     * Enum which defines the different LED behaviors.
     */
    enum class Behavior : uint8_t {
        Off = 0,
        On,
        Blinking,
        Flashing,
        Flickering
    };
    
    enum class DataType : uint8_t {
        String = 0,
        Byte,
        Word,
        Bool
    };
    
    enum class Group : uint8_t {
        Wifi = 0,
        Mqtt,
        Leds,
        Clock,
        Sensors,
        Bluetooth,
        _Last
    };
    
    struct ConfigEntry {
        Group                      group;
        const __FlashStringHelper* key;
        const __FlashStringHelper* label;
        const __FlashStringHelper* placeholder;
        DataType                   type;
        uint8_t                    maxLength;
        bool                       doNotDump;
        void*                      val;
    };

    /*
     * Defintion of default color names and hex values in a map like structure.
     */
    struct Map {
        const char* key;
        uint32_t    value;
    };

    static const constexpr Map DefaultColor[NUMBER_OF_DEFAULT_COLORS] = {
        {"NONE",   0x000000},
        {"GREEN",  0x00FF00},
        {"YELLOW", 0xFFCC00},
        {"ORANGE", 0xFF4400},
        {"RED",    0xFF0000},
        {"PURPLE", 0xFF00FF},
        {"BLUE",   0x0000FF},
        {"CYAN",   0x00FFFF},
        {"WHITE",  0xFFFFFF}
    };

    /*
     * This struct is used for mapping a device name to a led number, that means a specific position on the led stripe
     */
    struct DeviceMapping {
        DeviceMapping() {
            memset(name, 0, MAX_DEVICE_MAPPING_NAME_LEN);
            ledNumber = 0;
        }

        DeviceMapping(String n, int l) {
            strncpy(name, n.c_str(), MAX_DEVICE_MAPPING_NAME_LEN);
            name[MAX_DEVICE_MAPPING_NAME_LEN] = '\0';
            ledNumber = l;
        }

        int  ledNumber;                         // led number on which reactions for this device are displayed
        char name[MAX_DEVICE_MAPPING_NAME_LEN]; // name of the device
    };

    /*
     * This struct is used for mapping a message for a specific
     * message to a led behavior (see LedSwitcher::ledState).
     */
    struct ColorMapping {
        ColorMapping() {
            memset(msg, 0, MAX_COLOR_MAPPING_MSG_LEN);
            color = 0x000000;
            behavior = Behavior::Off;
        }

        ColorMapping(String m, uint32_t c, Behavior b) {
            strncpy(msg, m.c_str(), MAX_COLOR_MAPPING_MSG_LEN);
            msg[MAX_COLOR_MAPPING_MSG_LEN] = '\0';
            color = c;
            behavior = b;
        }

        Behavior behavior;                           // led behavior for message
        uint32_t color;                              // led color for message
        char     msg[MAX_COLOR_MAPPING_MSG_LEN + 1]; // message
    };

    HSDConfig();

    bool                        addColorMappingEntry(int entryNum, String msg, uint32_t color, Behavior behavior);
    bool                        addDeviceMappingEntry(int entryNum, String name, int ledNumber);
    void                        begin();
    inline const ConfigEntry*   cfgEntries() const { return m_cfgEntries; }
    void                        deleteAllColorMappingEntries();
    void                        deleteAllDeviceMappingEntries();
    bool                        deleteColorMappingEntry(int entryNum);
    bool                        deleteDeviceMappingEntry(int entryNum);
#if defined HSD_BLUETOOTH_ENABLED && defined ARDUINO_ARCH_ESP32
    inline bool                 getBluetoothEnabled() const { return m_cfgBluetoothEnabled; }
#endif
#ifdef HSD_CLOCK_ENABLED
    inline uint8_t              getClockBrightness() const { return m_cfgClockBrightness; }
    inline bool                 getClockEnabled() const { return m_cfgClockEnabled; }
    inline uint16_t             getClockNTPInterval() const { return m_cfgClockNTPInterval; }
    inline const String&        getClockNTPServer() const { return m_cfgClockNTPServer; }
    inline uint8_t              getClockPinCLK() const { return m_cfgClockPinCLK; }
    inline uint8_t              getClockPinDIO() const { return m_cfgClockPinDIO; }
    inline const String&        getClockTimeZone() const { return m_cfgClockTimeZone; }
#endif // HSD_CLOCK_ENABLED
    int                         getColorMapIndex(const String& msg) const;
    inline const ColorMapping*  getColorMapping(int index) { return m_cfgColorMapping.get(index); }
    uint32_t                    getDefaultColor(String key) const;
    String                      getDefaultColor(uint32_t value) const;
    String                      getDevice(int ledNumber) const;
    inline const DeviceMapping* getDeviceMapping(int index) const { return m_cfgDeviceMapping.get(index); }
    inline const String&        getHost() const { return m_cfgHost; }
    inline Behavior             getLedBehavior(int colorMapIndex) const { return m_cfgColorMapping.get(colorMapIndex)->behavior; }
    inline uint8_t              getLedBrightness() const { return m_cfgLedBrightness; }
    inline uint32_t             getLedColor(int colorMapIndex) const { return m_cfgColorMapping.get(colorMapIndex)->color; }
    inline uint8_t              getLedDataPin() const { return m_cfgLedDataPin; }
    int                         getLedNumber(const String& device) const;
    inline const String&        getMqttOutTopic() const { return m_cfgMqttOutTopic; }
    String                      getMqttOutTopic(const String& topic) const;
    inline const String&        getMqttPassword() const { return m_cfgMqttPassword; }
    inline uint16_t             getMqttPort() const { return m_cfgMqttPort; }
    inline const String&        getMqttServer() const { return m_cfgMqttServer; }
    inline const String&        getMqttStatusTopic() const { return m_cfgMqttStatusTopic; }
#ifdef MQTT_TEST_TOPIC
    inline const String&        getMqttTestTopic() const { return m_cfgMqttTestTopic; }
#endif    
    inline const String&        getMqttUser() const { return m_cfgMqttUser; }
    inline int                  getNumberOfColorMappingEntries() const { return m_cfgColorMapping.size(); }
    inline int                  getNumberOfDeviceMappingEntries() const { return m_cfgDeviceMapping.size(); }
    inline uint8_t              getNumberOfLeds() const { return m_cfgNumberOfLeds; }
#ifdef HSD_SENSOR_ENABLED
    inline bool                 getSensorSonoffEnabled() const { return m_cfgSensorSonoffEnabled; }
    inline uint16_t             getSensorInterval() const { return m_cfgSensorInterval; }
    inline uint8_t              getSensorPin() const { return m_cfgSensorPin; }
#endif // HSD_SENSOR_ENABLED
    inline const String&        getWifiPSK() const { return m_cfgWifiPSK; }
    inline const String&        getWifiSSID() const { return m_cfgWifiSSID; }
    String                      groupDescription(Group group) const;
    String                      hex2string(uint32_t value) const;
    inline bool                 isColorMappingDirty() const { return m_cfgColorMappingDirty; }
    inline bool                 isColorMappingFull() const { return m_cfgColorMapping.isFull(); }
    inline bool                 isDeviceMappingDirty() const { return m_cfgDeviceMappingDirty; }
    inline bool                 isDeviceMappingFull() const { return m_cfgDeviceMapping.isFull(); }
    inline void                 saveColorMapping() { writeColorMappingConfigFile(); }
    inline void                 saveDeviceMapping() { writeDeviceMappingConfigFile(); }
    inline void                 saveMain() { writeMainConfigFile(); }
    uint32_t                    string2hex(String value) const;
    inline void                 updateColorMapping() { readColorMappingConfigFile(); }
    inline void                 updateDeviceMapping() { readDeviceMappingConfigFile(); }

private:
    void onFileWriteError() const;
    void printMainConfigFile(JsonObject& json);
    bool readFile(const String& fileName, String& content) const;
    bool readColorMappingConfigFile();
    bool readDeviceMappingConfigFile();
    bool readMainConfigFile();
    bool writeFile(const String& fileName, JsonObject* data) const;
    void writeColorMappingConfigFile();
    void writeDeviceMappingConfigFile();
    void writeMainConfigFile() const;

#if defined HSD_BLUETOOTH_ENABLED && defined ARDUINO_ARCH_ESP32
    bool                                  m_cfgBluetoothEnabled;
#endif    
#ifdef HSD_CLOCK_ENABLED
    uint8_t                               m_cfgClockBrightness;
    bool                                  m_cfgClockEnabled;
    uint16_t                              m_cfgClockNTPInterval;
    String                                m_cfgClockNTPServer;
    uint8_t                               m_cfgClockPinCLK;
    uint8_t                               m_cfgClockPinDIO;
    String                                m_cfgClockTimeZone;
#endif // HSD_CLOCK_ENABLED
    PreAllocatedLinkedList<ColorMapping>  m_cfgColorMapping;
    bool                                  m_cfgColorMappingDirty;
    PreAllocatedLinkedList<DeviceMapping> m_cfgDeviceMapping;
    bool                                  m_cfgDeviceMappingDirty;
    String                                m_cfgHost;
    uint8_t                               m_cfgLedBrightness;
    uint8_t                               m_cfgLedDataPin;
    String                                m_cfgMqttOutTopic;
    String                                m_cfgMqttPassword;
    uint16_t                              m_cfgMqttPort;
    String                                m_cfgMqttServer;
    String                                m_cfgMqttStatusTopic;
#ifdef MQTT_TEST_TOPIC
    String                                m_cfgMqttTestTopic;
#endif // MQTT_TEST_TOPIC    
    String                                m_cfgMqttUser;
    uint8_t                               m_cfgNumberOfLeds;
#ifdef HSD_SENSOR_ENABLED
    bool                                  m_cfgSensorSonoffEnabled;
    uint16_t                              m_cfgSensorInterval;
    uint8_t                               m_cfgSensorPin;
#endif // HSD_SENSOR_ENABLED
    String                                m_cfgWifiPSK;
    String                                m_cfgWifiSSID;
    
    ConfigEntry*                          m_cfgEntries;
};

#endif // HSDCONFIG_H
