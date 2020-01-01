#ifndef HSDCONFIG_H
#define HSDCONFIG_H

#include <Arduino.h>
#include <ArduinoJson.h>

#include "QList.h"

#define HSD_VERSION         "0.9"
#define FILENAME_MAINCONFIG (F("/config.json"))

// comment out next line if you do not need the clock module
#define HSD_CLOCK_ENABLED
// comment out next line if you do not need the sensor module (Sonoff SI7021)
#define HSD_SENSOR_ENABLED
#define MQTT_TEST_TOPIC
#define HSD_BLUETOOTH_ENABLED

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
    
    /*
     * This struct is used for mapping a device name to a led number, that means a specific position on the led stripe
     */
    struct DeviceMapping {
        DeviceMapping(String n, uint8_t l) : device(n), ledNumber(l) { }

        String  device;    // name of the device
        uint8_t ledNumber; // led number on which reactions for this device are displayed
    };

    /*
     * This struct is used for mapping a message for a specific message to a led behavior (see LedSwitcher::ledState).
     */
    struct ColorMapping {
        ColorMapping(String m, uint32_t c, Behavior b) : behavior(b), color(c), msg(m) { }

        Behavior behavior; // led behavior for message
        uint32_t color;    // led color for message
        String   msg;      // message
    };

    enum class DataType : uint8_t {
        String = 0,
        Password,
        Bool,
        Gpio,
        Slider,
        Word,
        ColorMapping,
        DeviceMapping
    };
    
    enum class Group : uint8_t {
        Wifi = 0,
        Mqtt,
        Leds,
        Clock,
        Sensors,
        Bluetooth,
        __Last
    };
    
    struct ConfigEntry {
        ConfigEntry(Group g, const __FlashStringHelper* k, QList<ColorMapping>* v) : group(g), key(k), label(nullptr), pattern(nullptr), patternMsg(nullptr), type(DataType::ColorMapping), maxVal(0) { value.colMap = v; }
        ConfigEntry(Group g, const __FlashStringHelper* k, QList<DeviceMapping>* v) : group(g), key(k), label(nullptr), pattern(nullptr), patternMsg(nullptr), type(DataType::DeviceMapping), maxVal(0) { value.devMap = v; }
        ConfigEntry(Group g, const __FlashStringHelper* k, const __FlashStringHelper* l, const __FlashStringHelper* p, const __FlashStringHelper* pm, String* v, bool password = false) : group(g), key(k), label(l), pattern(p), patternMsg(pm), type(password ? DataType::Password : DataType::String), maxVal(0) { value.string = v; }
        ConfigEntry(Group g, const __FlashStringHelper* k, const __FlashStringHelper* l, bool* v) : group(g), key(k), label(l), pattern(nullptr), patternMsg(nullptr), type(DataType::Bool), maxVal(0) { value.boolean = v; }
        ConfigEntry(Group g, const __FlashStringHelper* k, const __FlashStringHelper* l, uint8_t* v) : group(g), key(k), label(l), pattern(nullptr), patternMsg(nullptr), type(DataType::Gpio), maxVal(0) { value.byte = v; }
        ConfigEntry(Group g, const __FlashStringHelper* k, const __FlashStringHelper* l, uint8_t max, uint8_t* v) : group(g), key(k), label(l), pattern(nullptr), patternMsg(nullptr), type(DataType::Slider), maxVal(max) { value.byte = v; }
        ConfigEntry(Group g, const __FlashStringHelper* k, const __FlashStringHelper* l, const __FlashStringHelper* p, const __FlashStringHelper* pm, uint16_t* v) : group(g), key(k), label(l), pattern(p), patternMsg(pm), type(DataType::Word), maxVal(5) { value.word = v; }
        
        Group                      group;
        const __FlashStringHelper* key;
        const __FlashStringHelper* label;
        const __FlashStringHelper* pattern;
        const __FlashStringHelper* patternMsg;
        DataType                   type;
        uint8_t                    maxVal;
        union {
            bool*                 boolean;
            uint8_t*              byte;
            QList<ColorMapping>*  colMap;
            QList<DeviceMapping>* devMap;
            String*               string;
            uint16_t*             word;
        } value;
    };

    /*
     * Defintion of default color names and hex values in a map like structure.
     */
    struct Map {
        const char* key;
        uint32_t    value;
    };

    HSDConfig();

    void                               begin();
    inline const QList<ConfigEntry>&   cfgEntries() { return m_cfgEntries; }
#if defined HSD_BLUETOOTH_ENABLED && defined ARDUINO_ARCH_ESP32
    inline bool                        getBluetoothEnabled() const { return m_cfgBluetoothEnabled; }
#endif
#ifdef HSD_CLOCK_ENABLED
    inline uint8_t                     getClockBrightness() const { return m_cfgClockBrightness; }
    inline bool                        getClockEnabled() const { return m_cfgClockEnabled; }
    inline uint8_t                     getClockNTPInterval() const { return m_cfgClockNTPInterval; }
    inline const String&               getClockNTPServer() const { return m_cfgClockNTPServer; }
    inline uint8_t                     getClockPinCLK() const { return m_cfgClockPinCLK; }
    inline uint8_t                     getClockPinDIO() const { return m_cfgClockPinDIO; }
    inline const String&               getClockTimeZone() const { return m_cfgClockTimeZone; }
#endif // HSD_CLOCK_ENABLED
    inline const QList<ColorMapping>&  getColorMap() const { return m_cfgColorMapping; }
    int                                getColorMapIndex(const String& msg) const;
    String                             getDevice(int ledNumber) const;
    inline const QList<DeviceMapping>& getDeviceMap() const { return m_cfgDeviceMapping; }
    inline const String&               getHost() const { return m_cfgHost; }
    inline Behavior                    getLedBehavior(unsigned int colorMapIndex) const { return m_cfgColorMapping[colorMapIndex].behavior; }
    inline uint8_t                     getLedBrightness() const { return m_cfgLedBrightness; }
    inline uint32_t                    getLedColor(unsigned int colorMapIndex) const { return m_cfgColorMapping[colorMapIndex].color; }
    inline uint8_t                     getLedDataPin() const { return m_cfgLedDataPin; }
    uint8_t                            getLedNumber(const String& device) const;
    inline const String&               getMqttOutTopic() const { return m_cfgMqttOutTopic; }
    String                             getMqttOutTopic(const String& topic) const;
    inline const String&               getMqttPassword() const { return m_cfgMqttPassword; }
    inline uint16_t                    getMqttPort() const { return m_cfgMqttPort; }
    inline const String&               getMqttServer() const { return m_cfgMqttServer; }
    inline const String&               getMqttStatusTopic() const { return m_cfgMqttStatusTopic; }
#ifdef MQTT_TEST_TOPIC
    inline const String&               getMqttTestTopic() const { return m_cfgMqttTestTopic; }
#endif    
    inline const String&               getMqttUser() const { return m_cfgMqttUser; }
    inline uint8_t                     getNumberOfLeds() const { return m_cfgNumberOfLeds; }
#ifdef HSD_SENSOR_ENABLED
    inline uint16_t                    getSensorAltitude() const { return m_cfgSensorAltitude; }
    inline bool                        getSensorI2CEnabled() const { return m_cfgSensorI2CEnabled; }
    inline uint8_t                     getSensorInterval() const { return m_cfgSensorInterval; }
    inline uint8_t                     getSensorPin() const { return m_cfgSensorPin; }
    inline bool                        getSensorSonoffEnabled() const { return m_cfgSensorSonoffEnabled; }
#endif // HSD_SENSOR_ENABLED
    inline const String&               getWifiPSK() const { return m_cfgWifiPSK; }
    inline const String&               getWifiSSID() const { return m_cfgWifiSSID; }
    String                             groupDescription(Group group) const;
    String                             hex2string(uint32_t value) const;
    bool                               readConfigFile();
    void                               setColorMap(QList<ColorMapping>& values);
    void                               setDeviceMap(QList<DeviceMapping>& values);
    uint32_t                           string2hex(String value) const;
    void                               writeConfigFile() const;

private:
#if defined HSD_BLUETOOTH_ENABLED && defined ARDUINO_ARCH_ESP32
    bool                 m_cfgBluetoothEnabled;
#endif    
#ifdef HSD_CLOCK_ENABLED
    uint8_t              m_cfgClockBrightness;
    bool                 m_cfgClockEnabled;
    uint8_t              m_cfgClockNTPInterval;
    String               m_cfgClockNTPServer;
    uint8_t              m_cfgClockPinCLK;
    uint8_t              m_cfgClockPinDIO;
    String               m_cfgClockTimeZone;
#endif // HSD_CLOCK_ENABLED
    QList<ColorMapping>  m_cfgColorMapping;
    QList<DeviceMapping> m_cfgDeviceMapping;
    String               m_cfgHost;
    uint8_t              m_cfgLedBrightness;
    uint8_t              m_cfgLedDataPin;
    String               m_cfgMqttOutTopic;
    String               m_cfgMqttPassword;
    uint16_t             m_cfgMqttPort;
    String               m_cfgMqttServer;
    String               m_cfgMqttStatusTopic;
#ifdef MQTT_TEST_TOPIC
    String               m_cfgMqttTestTopic;
#endif // MQTT_TEST_TOPIC    
    String               m_cfgMqttUser;
    uint8_t              m_cfgNumberOfLeds;
#ifdef HSD_SENSOR_ENABLED
    bool                 m_cfgSensorI2CEnabled;
    uint8_t              m_cfgSensorInterval;
    uint8_t              m_cfgSensorPin;
    bool                 m_cfgSensorSonoffEnabled;
    uint16_t             m_cfgSensorAltitude;
#endif // HSD_SENSOR_ENABLED
    String               m_cfgWifiPSK;
    String               m_cfgWifiSSID;
    
    QList<ConfigEntry>   m_cfgEntries;
};

#endif // HSDCONFIG_H
