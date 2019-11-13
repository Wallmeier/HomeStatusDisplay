#ifndef HSDCONFIG_H
#define HSDCONFIG_H

#include "HSDConfigFile.hpp"
#include "PreAllocatedLinkedList.hpp"

// comment out next line if you do not need the clock module
#define HSD_CLOCK_ENABLED
// comment out next line if you do not need the sensor module (Sonoff SI7021)
#define HSD_SENSOR_ENABLED

#define JSON_KEY_HOST                  (F("host"))
#define JSON_KEY_WIFI_SSID             (F("wifiSSID"))
#define JSON_KEY_WIFI_PSK              (F("wifiPSK"))
#define JSON_KEY_MQTT_SERVER           (F("mqttServer"))
#define JSON_KEY_MQTT_USER             (F("mqttUser"))
#define JSON_KEY_MQTT_PASSWORD         (F("mqttPassword"))
#define JSON_KEY_MQTT_STATUS_TOPIC     (F("mqttStatusTopic"))
#define JSON_KEY_MQTT_TEST_TOPIC       (F("mqttTestTopic"))
#define JSON_KEY_MQTT_WILL_TOPIC       (F("mqttWillTopic"))
#define JSON_KEY_LED_COUNT             (F("ledCount"))
#define JSON_KEY_LED_PIN               (F("ledPin"))
#define JSON_KEY_LED_BRIGHTNESS        (F("ledBrightness"))
#define JSON_KEY_CLOCK_PIN_CLK         (F("clockCLK"))
#define JSON_KEY_CLOCK_PIN_DIO         (F("clockDIO"))
#define JSON_KEY_CLOCK_BRIGHTNESS      (F("clockBrightness"))
#define JSON_KEY_CLOCK_TIME_ZONE       (F("clockTZ"))
#define JSON_KEY_CLOCK_NTP_SERVER      (F("clockServer"))
#define JSON_KEY_CLOCK_NTP_INTERVAL    (F("clockInterval"))
#ifdef HSD_SENSOR_ENABLED
#define JSON_KEY_SENSOR_ENABLED        (F("sensorEnabled"))
#define JSON_KEY_SENSOR_PIN            (F("sensorPin"))
#define JSON_KEY_SENSOR_INTERVAL       (F("sensorInterval"))
#endif // HSD_SENSOR_ENABLED
#define JSON_KEY_COLORMAPPING_MSG      (F("m"))
#define JSON_KEY_COLORMAPPING_COLOR    (F("c"))
#define JSON_KEY_COLORMAPPING_BEHAVIOR (F("b"))
#define JSON_KEY_DEVICEMAPPING_NAME    (F("n"))
#define JSON_KEY_DEVICEMAPPING_LED     (F("l"))

#define NUMBER_OF_DEFAULT_COLORS       9

class HSDConfig {
public:
    static const int MAX_DEVICE_MAPPING_NAME_LEN = 25;
    static const int MAX_COLOR_MAPPING_MSG_LEN = 15;

    /*
     * Enum which defines the different LED behaviors.
     */
    enum Behavior {
        OFF,
        ON,
        BLINKING,
        FLASHING,
        FLICKERING
    };

    /*
     * Defintion of default color names and hex values in a map like structure.
     */
    struct Map {
        char*    key;
        uint32_t value;
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
            behavior = OFF;
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
    void                        begin(const char* version, const char* defaultIdentifier);
    void                        deleteAllColorMappingEntries();
    void                        deleteAllDeviceMappingEntries();
    bool                        deleteColorMappingEntry(int entryNum);
    bool                        deleteDeviceMappingEntry(int entryNum);
#ifdef HSD_CLOCK_ENABLED
    inline uint8_t              getClockBrightness() const { return m_cfgClockBrightness; }
    inline uint16_t             getClockNTPInterval() const { return m_cfgClockNTPInterval; }
    inline const char*          getClockNTPServer() const { return m_cfgClockNTPServer; }
    inline uint8_t              getClockPinCLK() const { return m_cfgClockPinCLK; }
    inline uint8_t              getClockPinDIO() const { return m_cfgClockPinDIO; }
    inline const char*          getClockTimeZone() const { return m_cfgClockTimeZone; }
#endif // HSD_CLOCK_ENABLED
    int                         getColorMapIndex(const String& msg) const;
    inline const ColorMapping*  getColorMapping(int index) { return m_cfgColorMapping.get(index); }
    uint32_t                    getDefaultColor(String key) const;
    String                      getDefaultColor(uint32_t value) const;
    String                      getDevice(int ledNumber) const;
    inline const DeviceMapping* getDeviceMapping(int index) const { return m_cfgDeviceMapping.get(index); }
    inline const char*          getHost() const { return m_cfgHost; }
    inline Behavior             getLedBehavior(int colorMapIndex) const { return m_cfgColorMapping.get(colorMapIndex)->behavior; }
    inline uint8_t              getLedBrightness() const { return m_cfgLedBrightness; }
    inline uint32_t             getLedColor(int colorMapIndex) const { return m_cfgColorMapping.get(colorMapIndex)->color; }
    inline uint8_t              getLedDataPin() const { return m_cfgLedDataPin; }
    int                         getLedNumber(const String& device) const;
    inline const char*          getMqttPassword() const { return m_cfgMqttPassword; }
    inline const char*          getMqttServer() const { return m_cfgMqttServer; }
    inline const char*          getMqttStatusTopic() const { return m_cfgMqttStatusTopic; }
    inline const char*          getMqttTestTopic() const { return m_cfgMqttTestTopic; }
    inline const char*          getMqttUser() const { return m_cfgMqttUser; }
    inline const char*          getMqttWillTopic() const { return m_cfgMqttWillTopic; }
    inline int                  getNumberOfColorMappingEntries() const { return m_cfgColorMapping.size(); }
    inline int                  getNumberOfDeviceMappingEntries() const { return m_cfgDeviceMapping.size(); }
    inline uint8_t              getNumberOfLeds() const { return m_cfgNumberOfLeds; }
#ifdef HSD_SENSOR_ENABLED
    inline bool                 getSensorEnabled() const { return m_cfgSensorEnabled; }
    inline uint16_t             getSensorInterval() const { return m_cfgSensorInterval; }
    inline uint8_t              getSensorPin() const { return m_cfgSensorPin; }
#endif // HSD_SENSOR_ENABLED
    inline const char*          getVersion() const { return m_cfgVersion; }
    inline const char*          getWifiPSK() const { return m_cfgWifiPSK; }
    inline const char*          getWifiSSID() const { return m_cfgWifiSSID; }
    String                      hex2string(uint32_t value) const;
    inline bool                 isColorMappingDirty() const { return m_cfgColorMappingDirty; }
    inline bool                 isColorMappingFull() const { return m_cfgColorMapping.isFull(); }
    inline bool                 isDeviceMappingDirty() const { return m_cfgDeviceMappingDirty; }
    inline bool                 isDeviceMappingFull() const { return m_cfgDeviceMapping.isFull(); }
    inline void                 saveColorMapping() { writeColorMappingConfigFile(); }
    inline void                 saveDeviceMapping() { writeDeviceMappingConfigFile(); }
    inline void                 saveMain() { writeMainConfigFile(); }
#ifdef HSD_CLOCK_ENABLED
    inline bool                 setClockBrightness(uint8_t brightness) { return setValue<uint8_t>(m_cfgClockBrightness, brightness); }
    inline bool                 setClockNTPInterval(uint16_t minutes) { return setValue<uint16_t>(m_cfgClockNTPInterval, minutes); }
    inline bool                 setClockNTPServer(const char* server) { return setStringValue(m_cfgClockNTPServer, MAX_CLOCK_NTP_SERVER_LEN, server); }
    inline bool                 setClockPinCLK(uint8_t dataPin) { return setValue<uint8_t>(m_cfgClockPinCLK, dataPin); }
    inline bool                 setClockPinDIO(uint8_t dataPin) { return setValue<uint8_t>(m_cfgClockPinDIO, dataPin); }
    inline bool                 setClockTimeZone(const char* zone) { return setStringValue(m_cfgClockTimeZone, MAX_CLOCK_TIMEZONE_LEN, zone); }
#endif // HSD_CLOCK_ENABLED
    inline bool                 setHost(const char* host) { return setStringValue(m_cfgHost, MAX_HOST_LEN, host); }
    inline bool                 setLedBrightness(uint8_t brightness) { return setValue<uint8_t>(m_cfgLedBrightness, brightness); }
    inline bool                 setLedDataPin(uint8_t dataPin) { return setValue<uint8_t>(m_cfgLedDataPin, dataPin); }
    inline bool                 setMqttPassword(const char* pwd) { return setStringValue(m_cfgMqttPassword, MAX_MQTT_PASSWORD_LEN, pwd); }
    inline bool                 setMqttServer(const char* ip) { return setStringValue(m_cfgMqttServer, MAX_MQTT_SERVER_LEN, ip); }
    inline bool                 setMqttStatusTopic(const char* topic) { return setStringValue(m_cfgMqttStatusTopic, MAX_MQTT_STATUS_TOPIC_LEN, topic); }
    inline bool                 setMqttTestTopic(const char* topic) { return setStringValue(m_cfgMqttTestTopic, MAX_MQTT_TEST_TOPIC_LEN, topic); }
    inline bool                 setMqttUser(const char* user) { return setStringValue(m_cfgMqttUser, MAX_MQTT_USER_LEN, user); }
    inline bool                 setMqttWillTopic(const char* topic) { return setStringValue(m_cfgMqttWillTopic, MAX_MQTT_WILL_TOPIC_LEN, topic); }
    inline bool                 setNumberOfLeds(uint8_t numberOfLeds) { return setValue<uint8_t>(m_cfgNumberOfLeds, numberOfLeds); }
#ifdef HSD_SENSOR_ENABLED
    inline bool                 setSensorEnabled(bool enabled) { setValue<bool>(m_cfgSensorEnabled, enabled); }
    inline bool                 setSensorInterval(uint16_t interval) { setValue<uint16_t>(m_cfgSensorInterval, interval); }
    inline bool                 setSensorPin(uint8_t pin) { setValue<uint8_t>(m_cfgSensorPin, pin); }
#endif // HSD_SENSOR_ENABLED
    inline bool                 setVersion(const char* version) { return setStringValue(m_cfgVersion, MAX_VERSION_LEN, version); }
    inline bool                 setWifiPSK(const char* psk) { return setStringValue(m_cfgWifiPSK, MAX_WIFI_PSK_LEN, psk); }
    inline bool                 setWifiSSID(const char* ssid) { return setStringValue(m_cfgWifiSSID, MAX_WIFI_SSID_LEN, ssid); }
    uint32_t                    string2hex(String value) const;
    inline void                 updateColorMapping() { readColorMappingConfigFile(); }
    inline void                 updateDeviceMapping() { readDeviceMappingConfigFile(); }

private:
    void onFileWriteError();
    void printMainConfigFile(JsonObject& json);
    bool readColorMappingConfigFile();
    bool readDeviceMappingConfigFile();
    bool readMainConfigFile();
    void resetColorMappingConfigData();
    void resetDeviceMappingConfigData();
    void resetMainConfigData();
    bool setStringValue(char* val, int maxLen, const char* newValue) const;
    template <class T>
    bool setValue(T& val, const T& newValue) const {
        if (val != newValue) {
            val = newValue;
            return true;
        } else {
            return false;
        }
    }
    void writeColorMappingConfigFile();
    void writeDeviceMappingConfigFile();
    void writeMainConfigFile();

    static const int MAX_CLOCK_NTP_SERVER_LEN  = 40;
    static const int MAX_CLOCK_TIMEZONE_LEN    = 40;
    static const int MAX_COLOR_MAP_ENTRIES     = 30;
    static const int MAX_DEVICE_MAP_ENTRIES    = 35;
    static const int MAX_HOST_LEN              = 30;
    static const int MAX_MQTT_PASSWORD_LEN     = 50;
    static const int MAX_MQTT_SERVER_LEN       = 20;
    static const int MAX_MQTT_STATUS_TOPIC_LEN = 50;
    static const int MAX_MQTT_TEST_TOPIC_LEN   = 50;
    static const int MAX_MQTT_USER_LEN         = 20;
    static const int MAX_MQTT_WILL_TOPIC_LEN   = 50;
    static const int MAX_VERSION_LEN           = 20;
    static const int MAX_WIFI_PSK_LEN          = 30;
    static const int MAX_WIFI_SSID_LEN         = 30;

#ifdef HSD_CLOCK_ENABLED
    uint8_t                               m_cfgClockBrightness;
    uint16_t                              m_cfgClockNTPInterval;
    char                                  m_cfgClockNTPServer[MAX_CLOCK_NTP_SERVER_LEN + 1];
    uint8_t                               m_cfgClockPinCLK;
    uint8_t                               m_cfgClockPinDIO;
    char                                  m_cfgClockTimeZone[MAX_CLOCK_TIMEZONE_LEN + 1];
#endif // HSD_CLOCK_ENABLED
    PreAllocatedLinkedList<ColorMapping>  m_cfgColorMapping;
    bool                                  m_cfgColorMappingDirty;
    PreAllocatedLinkedList<DeviceMapping> m_cfgDeviceMapping;
    bool                                  m_cfgDeviceMappingDirty;
    char                                  m_cfgHost[MAX_HOST_LEN + 1];
    uint8_t                               m_cfgLedBrightness;
    uint8_t                               m_cfgLedDataPin;
    char                                  m_cfgMqttPassword[MAX_MQTT_PASSWORD_LEN + 1];
    char                                  m_cfgMqttServer[MAX_MQTT_SERVER_LEN + 1];
    char                                  m_cfgMqttStatusTopic[MAX_MQTT_STATUS_TOPIC_LEN + 1];
    char                                  m_cfgMqttTestTopic[MAX_MQTT_TEST_TOPIC_LEN + 1];
    char                                  m_cfgMqttUser[MAX_MQTT_USER_LEN + 1];
    char                                  m_cfgMqttWillTopic[MAX_MQTT_WILL_TOPIC_LEN + 1];
    uint8_t                               m_cfgNumberOfLeds;
#ifdef HSD_SENSOR_ENABLED
    bool                                  m_cfgSensorEnabled;
    uint16_t                              m_cfgSensorInterval;
    uint8_t                               m_cfgSensorPin;
#endif // HSD_SENSOR_ENABLED
    char                                  m_cfgVersion[MAX_VERSION_LEN + 1];
    char                                  m_cfgWifiPSK[MAX_WIFI_PSK_LEN + 1];
    char                                  m_cfgWifiSSID[MAX_WIFI_SSID_LEN + 1];
    HSDConfigFile                         m_colorMappingConfigFile;
    HSDConfigFile                         m_deviceMappingConfigFile;
    HSDConfigFile                         m_mainConfigFile;
};

#endif // HSDCONFIG_H
