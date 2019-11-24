#include "HomeStatusDisplay.hpp"

#include <ArduinoJson.h>

#define ONE_MINUTE_MILLIS (60000)

// ---------------------------------------------------------------------------------------------------------------------

HomeStatusDisplay::HomeStatusDisplay() :
#if defined HSD_BLUETOOTH_ENABLED && defined ARDUINO_ARCH_ESP32
    m_bluetooth(nullptr),
#endif    
#ifdef HSD_CLOCK_ENABLED
    m_clock(nullptr),
#endif
    m_leds(m_config),
    m_mqttHandler(m_config, std::bind(&HomeStatusDisplay::mqttCallback, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)),
#ifdef HSD_SENSOR_ENABLED
    m_sensor(nullptr),
#endif
    m_webServer(m_config, m_leds, m_mqttHandler),
    m_wifi(m_config)
{
}

// ---------------------------------------------------------------------------------------------------------------------

void HomeStatusDisplay::begin() {
    // initialize serial
    Serial.begin(115200);
    Serial.println(F(""));

    m_config.begin();
    m_leds.begin();
    m_wifi.begin();
    m_webServer.begin();
    m_mqttHandler.begin();
#ifdef HSD_CLOCK_ENABLED
    if (m_config.getClockEnabled()) {
        m_clock = new HSDClock(m_config);
        m_clock->begin();
    }
#endif
#ifdef HSD_SENSOR_ENABLED
    if (m_config.getSensorSonoffEnabled()) {
        m_sensor = new HSDSensor(m_config);
        m_sensor->begin();
    }
#endif
#if defined HSD_BLUETOOTH_ENABLED && defined ARDUINO_ARCH_ESP32
    if (m_config.getBluetoothEnabled()) {
        m_bluetooth = new HSDBluetooth(m_config, m_mqttHandler);
        m_bluetooth->begin();
    }
#endif    
    Serial.print(F("Free RAM: ")); 
    Serial.println(ESP.getFreeHeap());
}

// ---------------------------------------------------------------------------------------------------------------------

void HomeStatusDisplay::work() {
    checkConnections();
    m_wifi.handleConnection();
    m_webServer.handleClient(calcUptime());

    if (m_wifi.connected())
        m_mqttHandler.handle();
  
    m_leds.update();
#ifdef HSD_CLOCK_ENABLED
    if (m_clock)
        m_clock->handle();
#endif
#if defined HSD_BLUETOOTH_ENABLED && defined ARDUINO_ARCH_ESP32
    if (m_bluetooth)
        m_bluetooth->handle();
#endif    
    delay(1);
}

// ---------------------------------------------------------------------------------------------------------------------

unsigned long HomeStatusDisplay::calcUptime() {
    static unsigned long oneMinuteTimerLast = 0;
    static unsigned long uptime = 0;
#ifdef HSD_SENSOR_ENABLED
    static uint16_t sensorMinutes = 0;
#endif  
    if (millis() - oneMinuteTimerLast >= ONE_MINUTE_MILLIS) {
        uptime++;
        oneMinuteTimerLast = millis();

        Serial.println("Uptime: " + String(uptime) + " min");

        if (m_mqttHandler.connected()) {
            String topic = m_config.getMqttOutTopic("statistic");
            if (m_mqttHandler.isTopicValid(topic)) {
                DynamicJsonBuffer jsonBuffer;
                JsonObject& json = jsonBuffer.createObject();
                json["Uptime"] = uptime;
#ifdef ARDUINO_ARCH_ESP32
                json["HeapFree"] = ESP.getFreeHeap();
                json["HeapMinFree"] = ESP.getMinFreeHeap();
                json["HeapSize"] = ESP.getHeapSize();
#else
                uint32_t free;
                uint16_t max;
                uint8_t frag;
                ESP.getHeapStats(&free, &max, &frag);
                
                json["HeapFree"] = free;
                json["HeapMax"] = max;
                json["HeapFrag"] = frag;
#endif // ARDUINO_ARCH_ESP32
                if (WiFi.status() == WL_CONNECTED)
                    json["RSSI"] = WiFi.RSSI();
                m_mqttHandler.publish(topic, json);
            }
        }
    
#ifdef HSD_SENSOR_ENABLED
        if (m_sensor) {
            sensorMinutes++;
            if (sensorMinutes >= m_config.getSensorInterval()) {
                sensorMinutes = 0;
                float temp, hum;
                if (m_sensor->readSensor(temp, hum)) {
                    m_webServer.setSensorData(temp, hum);
                    Serial.print(F("Sensor: Temp "));
                    Serial.print(temp, 1);
                    Serial.print(F("°C, Hum "));
                    Serial.print(hum, 1);
                    Serial.println(F("%"));
          
                    if (m_mqttHandler.connected()) {
                        String topic = m_config.getMqttOutTopic("sensor");
                        if (m_mqttHandler.isTopicValid(topic)) {
                            DynamicJsonBuffer jsonBuffer;
                            JsonObject& json = jsonBuffer.createObject();
                            json["Temp"] = temp;
                            json["Hum"] = hum;
                            m_mqttHandler.publish(topic, json);
                        }
                    }
                } else {
                    Serial.println(F("Sensor failed"));
                }
            }
        }
#endif
    }
    return uptime;
}

// ---------------------------------------------------------------------------------------------------------------------

void HomeStatusDisplay::mqttCallback(char* topic, byte* payload, unsigned int length) {
    String mqttTopicString(topic);
    String mqttMsgString;

    for (unsigned int idx = 0; idx < length; idx++)
        mqttMsgString += (char)payload[idx];
  
    Serial.print(F("Received an MQTT message for topic ")); Serial.println(mqttTopicString + ": " + mqttMsgString);

#ifdef MQTT_TEST_TOPIC    
    if (mqttTopicString.equals(m_config.getMqttTestTopic())) {
        handleTest(mqttMsgString);
    } else 
#endif // MQTT_TEST_TOPIC    
    if (isStatusTopic(mqttTopicString)) {
        String device = getDevice(mqttTopicString);
        handleStatus(device, mqttMsgString);
    }
}

// ---------------------------------------------------------------------------------------------------------------------

bool HomeStatusDisplay::isStatusTopic(const String& topic) const {
    String mqttStatusTopic = String(m_config.getMqttStatusTopic());
    int posOfLastSlashInStatusTopic = mqttStatusTopic.lastIndexOf("/");
    return topic.startsWith(mqttStatusTopic.substring(0, posOfLastSlashInStatusTopic)) ? true : false;
}

// ---------------------------------------------------------------------------------------------------------------------

String HomeStatusDisplay::getDevice(const String& statusTopic) const {
    int posOfLastSlashInStatusTopic = statusTopic.lastIndexOf("/");
    return statusTopic.substring(posOfLastSlashInStatusTopic + 1);
}

// ---------------------------------------------------------------------------------------------------------------------
#ifdef MQTT_TEST_TOPIC
void HomeStatusDisplay::handleTest(const String& msg) {
    int type(msg.toInt());
    if (type > 0) {
        Serial.print(F("Showing testpattern ")); 
        Serial.println(type);
        m_leds.test(type);
    } else if (type == 0) {
        m_leds.clear();
        m_mqttHandler.reconnect();  // back to normal
    }
}
#endif // MQTT_TEST_TOPIC
// ---------------------------------------------------------------------------------------------------------------------

void HomeStatusDisplay::handleStatus(const String& device, const String& msg) { 
    int ledNumber(m_config.getLedNumber(device));
    if (ledNumber != -1) {
        int colorMapIndex(m_config.getColorMapIndex(msg));    
        if (colorMapIndex != -1) {
            HSDConfig::Behavior behavior = m_config.getLedBehavior(colorMapIndex);
            uint32_t color = m_config.getLedColor(colorMapIndex);

            Serial.println("Set led number " + String(ledNumber) + " to behavior " + String(static_cast<uint8_t>(behavior)) + " with color " + m_config.hex2string(color));
            m_leds.set(ledNumber, behavior, color);
        } else if (msg.length() > 3 && msg[0] == '#') {  // allow MQTT broker to directly set LED color with HEX strings
            uint32_t color = m_config.string2hex(msg);
            Serial.println("Received HEX " + String(msg) + " and set led number " + String(ledNumber) + " with this color ON");
            m_leds.set(ledNumber, HSDConfig::Behavior::On, color);
        } else {
            Serial.println("Unknown message " + msg + " for led number " + String(ledNumber) + ", set to OFF");
            m_leds.set(ledNumber, HSDConfig::Behavior::Off, m_config.getDefaultColor("NONE"));
        }
    } else {
        Serial.println("No LED defined for device " + device + ", ignoring it");
    }
}

// ---------------------------------------------------------------------------------------------------------------------

void HomeStatusDisplay::checkConnections() {
    static bool lastMqttConnectionState = false;
    static bool lastWifiConnectionState = false;
    
    if (!lastMqttConnectionState && m_mqttHandler.connected()) {
        m_leds.clear();
        lastMqttConnectionState = true;
    } else if (lastMqttConnectionState && !m_mqttHandler.connected()) {
        m_leds.clear();
        lastMqttConnectionState = false;
    }

    if (!m_mqttHandler.connected() && m_wifi.connected())
        m_leds.setAll(HSDConfig::Behavior::On, m_config.getDefaultColor("YELLOW"));
  
    if (!lastWifiConnectionState && m_wifi.connected()) {
        m_leds.clear();
        if (!m_mqttHandler.connected())
            m_leds.setAll(HSDConfig::Behavior::On, m_config.getDefaultColor("ORANGE"));
        lastWifiConnectionState = true;
    } else if (lastWifiConnectionState && !m_wifi.connected()) {
        m_leds.clear();
        lastWifiConnectionState = false;
    }

    if (!m_wifi.connected())
        m_leds.setAll(HSDConfig::Behavior::On, m_config.getDefaultColor("RED"));
}
