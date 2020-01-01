#include "HomeStatusDisplay.hpp"

#include <ArduinoJson.h>
#include <ArduinoOTA.h>

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
    m_wifi(m_config, m_leds, m_webServer)
{
}

// ---------------------------------------------------------------------------------------------------------------------

void HomeStatusDisplay::begin() {
    // initialize serial
    Serial.begin(115200);
    Serial.println(F(""));

    m_config.begin();
    ArduinoOTA.setHostname(m_config.getHost().c_str());
    ArduinoOTA.onStart([=]() {
        String type;
        if (ArduinoOTA.getCommand() == U_FLASH) {
            type = "sketch";
        } else { // U_FS
            // NOTE: if updating FS this would be the place to unmount FS using FS.end()
            type = "filesystem";
            SPIFFS.end();
        }
        Serial.println("ArduinoOTA: start updating " + type);
        m_leds.setAll(HSDConfig::Behavior::On, LED_COLOR_BLUE);
    });
    ArduinoOTA.onEnd([=]() {
        Serial.println("ArduinoOTA: end");
        m_leds.clear();
    });
    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
        static int val = 0;
        int newVal = progress / (total / 100);
        if (newVal != val) {
            Serial.printf("ArduinoOTA: progress: %u%%\n", newVal);
            val = newVal;
        }
    });
    ArduinoOTA.onError([=](ota_error_t error) {
        Serial.printf("ArduinoOTA: error[%u]: ", error);
        if (error == OTA_AUTH_ERROR)
            Serial.println("Auth Failed");
        else if (error == OTA_BEGIN_ERROR)
            Serial.println("Begin Failed");
        else if (error == OTA_CONNECT_ERROR)
            Serial.println("Connect Failed");
        else if (error == OTA_RECEIVE_ERROR)
            Serial.println("Receive Failed");
        else if (error == OTA_END_ERROR)
            Serial.println("End Failed");
        m_leds.clear();
    });    
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
    if (m_config.getSensorSonoffEnabled() || m_config.getSensorI2CEnabled()) {
        m_sensor = new HSDSensor(m_config);
        m_sensor->begin(m_webServer);
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
    checkMqttConnections();
    m_wifi.handleConnection();
    calcUptime();

    if (WiFi.isConnected()) {
        m_mqttHandler.handle();
        ArduinoOTA.handle();
    }
  
    m_leds.update();
#ifdef HSD_CLOCK_ENABLED
    if (m_clock)
        m_clock->handle();
#endif
#if defined HSD_BLUETOOTH_ENABLED && defined ARDUINO_ARCH_ESP32
    if (m_bluetooth)
        m_bluetooth->handle();
#endif
#ifdef HSD_SENSOR_ENABLED
    if (m_config.getSensorSonoffEnabled() || m_config.getSensorI2CEnabled())
        m_sensor->handle(m_webServer, m_mqttHandler);
#endif // HSD_SENSOR_ENABLED
    delay(10);
}

// ---------------------------------------------------------------------------------------------------------------------

void HomeStatusDisplay::calcUptime() {
    static unsigned long oneMinuteTimerLast = 0;
    static unsigned long uptime = 0;

    if (millis() - oneMinuteTimerLast >= ONE_MINUTE_MILLIS) {
        uptime++;
        oneMinuteTimerLast = millis();
        m_webServer.setUptime(uptime);
        
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
    }
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
        handleStatus(getDevice(mqttTopicString), mqttMsgString);
        m_webServer.ledChange();
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
            m_leds.set(ledNumber, HSDConfig::Behavior::Off, LED_COLOR_NONE);
        }
    } else {
        Serial.println("No LED defined for device " + device + ", ignoring it");
    }
}

// ---------------------------------------------------------------------------------------------------------------------

void HomeStatusDisplay::checkMqttConnections() {
    static bool lastMqttConnectionState = false;
    
    if (!lastMqttConnectionState && m_mqttHandler.connected()) {
        lastMqttConnectionState = true;
        m_webServer.updateStatusEntry(F("mqttStatus"), "connected");
        m_leds.clear();
    } else if (lastMqttConnectionState && !m_mqttHandler.connected()) {
        lastMqttConnectionState = false;
        m_webServer.updateStatusEntry(F("mqttStatus"), "disconnected");
        m_leds.setAll(HSDConfig::Behavior::On, LED_COLOR_YELLOW);
    }
}
