#include "HSDMqtt.hpp"

HSDMqtt::HSDMqtt(const HSDConfig& config, MQTT_CALLBACK_SIGNATURE) :
    m_config(config),
    m_numberOfInTopics(0),
    m_pubSubClient(m_wifiClient)
{
    m_pubSubClient.setCallback(callback);
}

// ---------------------------------------------------------------------------------------------------------------------

void HSDMqtt::begin() {  
    addTopic(m_config.getMqttStatusTopic());
#ifdef MQTT_TEST_TOPIC  
    addTopic(m_config.getMqttTestTopic());
#endif // MQTT_TEST_TOPIC  

    IPAddress mqttIpAddr;
    if (mqttIpAddr.fromString(m_config.getMqttServer())) // valid ip address entered 
        m_pubSubClient.setServer(mqttIpAddr, m_config.getMqttPort()); 
     else                                                // invalid ip address, try as hostname
        m_pubSubClient.setServer(m_config.getMqttServer().c_str(), m_config.getMqttPort());
}

// ---------------------------------------------------------------------------------------------------------------------

void HSDMqtt::handle() {
    static bool first = true;
    static unsigned long millisLastConnectTry = 0;
    
    if (!m_pubSubClient.loop() && WiFi.isConnected()) {
        if (first || ((millis() - millisLastConnectTry) >= 10000)) { // alle 10 Sekunden testen
            first = false;
            millisLastConnectTry = millis();
            reconnect();
        }
    }
}

// ---------------------------------------------------------------------------------------------------------------------

bool HSDMqtt::reconnect() const {
    bool retval(false), connected(false);
  
    // Create a constant but unique client ID
    String mac = WiFi.macAddress();
    mac.replace(":", "");
    char clientId[24];
    snprintf(clientId, 24, "%s-%s", m_config.getHost().c_str(), mac.substring(6).c_str());
    Serial.printf("Connecting to MQTT broker %s:%d with clientId %s...", m_config.getMqttServer().c_str(), 
                  m_config.getMqttPort(), clientId);

    const String& willTopic = m_config.getMqttOutTopic("status");
    if (m_config.getMqttUser().length() == 0) {
        if (isTopicValid(willTopic))
            connected = m_pubSubClient.connect(clientId, willTopic.c_str(), 0, true, "off");
        else
            connected = m_pubSubClient.connect(clientId);
    } else {
        if (isTopicValid(willTopic))
            connected = m_pubSubClient.connect(clientId, m_config.getMqttUser().c_str(), m_config.getMqttPassword().c_str(), willTopic.c_str(), 0, true, "off");
        else
            connected = m_pubSubClient.connect(clientId, m_config.getMqttUser().c_str(), m_config.getMqttPassword().c_str());
    }
    if (connected) {
        Serial.println(F("connected"));
        if (isTopicValid(willTopic))
            publish(willTopic, "on");
        String verTopic = m_config.getMqttOutTopic("versions");
        if (isTopicValid(verTopic)) {
            DynamicJsonBuffer jsonBuffer;
            JsonObject& json = jsonBuffer.createObject();
            json["Firmware"] = String(HSD_VERSION);
#ifndef ARDUINO_ARCH_ESP32
            json["CoreVersion"] = ESP.getCoreVersion();
#endif            
            json["SdkVersion"] = ESP.getSdkVersion();
            publish(verTopic, json);
        }
        for (uint32_t index = 0; index < m_numberOfInTopics; index++)
            subscribe(m_inTopics[index]);
        retval = true;
    } else {
        Serial.printf("failed, rc = %d\n", m_pubSubClient.state());
    }
    return retval;
}

// ---------------------------------------------------------------------------------------------------------------------

void HSDMqtt::subscribe(const String& topic) const {
    if (isTopicValid(topic)) {
        Serial.print(F("Subscribing to topic "));
        Serial.println(topic);
        if (!m_pubSubClient.subscribe(topic.c_str())) {
            Serial.print("Failed to subscribe to topic ");
            Serial.println(topic);
        }
    }
}

// ---------------------------------------------------------------------------------------------------------------------

void HSDMqtt::publish(const String& topic, String msg) const {
    if (connected()) {
        if (m_pubSubClient.publish(topic.c_str(), msg.c_str()))
            Serial.println("Published msg " + msg + " for topic " + topic);
        else
            Serial.println("Error publishing msg " + msg + " for topic " + topic + " - rc: " + String(m_pubSubClient.state()));
    } else {
        Serial.println("Not connected - failed to publish msg " + msg + " for topic " + topic);
    }
}

// ---------------------------------------------------------------------------------------------------------------------

void HSDMqtt::publish(const String& topic, const JsonObject& json) const {
    String jsonStr;
    json.printTo(jsonStr);
    publish(topic, jsonStr);
}

// ---------------------------------------------------------------------------------------------------------------------

bool HSDMqtt::addTopic(const String& topic) {
    if (isTopicValid(topic) && m_numberOfInTopics < (MAX_IN_TOPICS - 1)) {
        m_inTopics[m_numberOfInTopics] = topic;
        m_numberOfInTopics++;
        return true;
    }
    return false;
}
