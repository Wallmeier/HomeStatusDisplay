#include "HSDMqtt.hpp"
#include "HSDLogger.hpp"

HSDMqtt::HSDMqtt(const HSDConfig* config, MQTT_CALLBACK_SIGNATURE) :
    m_config(config),
    m_pubSubClient(new PubSubClient(m_wifiClient))
{
    m_pubSubClient->setCallback(callback);
}

// ---------------------------------------------------------------------------------------------------------------------

void HSDMqtt::begin() {  
    IPAddress mqttIpAddr;
    if (mqttIpAddr.fromString(m_config->getMqttServer())) // valid ip address entered 
        m_pubSubClient->setServer(mqttIpAddr, m_config->getMqttPort()); 
     else                                                // invalid ip address, try as hostname
        m_pubSubClient->setServer(m_config->getMqttServer().c_str(), m_config->getMqttPort());
}

// ---------------------------------------------------------------------------------------------------------------------

void HSDMqtt::handle() {
    static bool first = true;
    static unsigned long millisLastConnectTry = 0;
    
    if (WiFi.isConnected()) {
        if (m_pubSubClient->connected()) {
            if (!m_pubSubClient->loop()) 
                Logger.log("Mqtt disconnected - state=%d", m_pubSubClient->state());
        } else {
            if (first || ((millis() - millisLastConnectTry) >= 10000)) { // alle 10 Sekunden testen
                Logger.log("Mqtt not connected (state=%d)", m_pubSubClient->state());
                first = false;
                millisLastConnectTry = millis();
                reconnect();
            }
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
    snprintf(clientId, 24, "%s-%s", m_config->getHost().c_str(), mac.substring(6).c_str());

    const String& willTopic = m_config->getMqttOutTopic("status");
    m_pubSubClient->disconnect();
    if (m_config->getMqttUser().length() == 0) {
        if (isTopicValid(willTopic))
            connected = m_pubSubClient->connect(clientId, willTopic.c_str(), 0, true, "offline");
        else
            connected = m_pubSubClient->connect(clientId);
    } else {
        if (isTopicValid(willTopic))
            connected = m_pubSubClient->connect(clientId, m_config->getMqttUser().c_str(), m_config->getMqttPassword().c_str(), willTopic.c_str(), 0, true, "off");
        else
            connected = m_pubSubClient->connect(clientId, m_config->getMqttUser().c_str(), m_config->getMqttPassword().c_str());
    }
    if (connected) {
        Logger.log("Connected to MQTT broker %s:%d with clientId %s", m_config->getMqttServer().c_str(), 
                   m_config->getMqttPort(), clientId);
        if (isTopicValid(willTopic))
            publish(willTopic, "online");
        String verTopic = m_config->getMqttOutTopic("versions");
        if (isTopicValid(verTopic)) {
            DynamicJsonBuffer jsonBuffer;
            JsonObject& json = jsonBuffer.createObject();
            json["Firmware"] = String(HSD_VERSION);
#ifdef ESP8266
            json["CoreVersion"] = ESP.getCoreVersion();
#endif            
            json["SdkVersion"] = ESP.getSdkVersion();
            publish(verTopic, json);
        }
        subscribe(m_config->getMqttStatusTopic());
#ifdef MQTT_TEST_TOPIC  
        subscribe(m_config->getMqttTestTopic());
#endif // MQTT_TEST_TOPIC  
        retval = true;
    } else {
        Logger.log("Failed to connect to MQTT broker %s:%d, rc=%d", m_config->getMqttServer().c_str(), 
                   m_config->getMqttPort(), m_pubSubClient->state());
    }
    return retval;
}

// ---------------------------------------------------------------------------------------------------------------------

void HSDMqtt::subscribe(const String& topic) const {
    if (isTopicValid(topic)) {
        if (!m_pubSubClient->subscribe(topic.c_str()))
            Logger.log("Failed to subscribe to topic %s", topic.c_str());
        else
            Logger.log("Subscribed to topic %s", topic.c_str());
    }
}

// ---------------------------------------------------------------------------------------------------------------------

void HSDMqtt::publish(const String& topic, String msg) const {
    if (connected()) {
        if (m_pubSubClient->publish(topic.c_str(), msg.c_str()))
            Logger.log("Published msg %s for topic %s (free RAM %u)", msg.c_str(), topic.c_str(), ESP.getFreeHeap());
        else
            Logger.log("Error publishing msg %s for topic %s (free RAM %u) - rc: %d", msg.c_str(), topic.c_str(), 
                       ESP.getFreeHeap(), m_pubSubClient->state());
    } else {
        Logger.log("Not connected - failed to publish msg %s for topic %s", msg.c_str(), topic.c_str());
    }
}

// ---------------------------------------------------------------------------------------------------------------------

void HSDMqtt::publish(const String& topic, const JsonObject& json) const {
    String jsonStr;
    json.printTo(jsonStr);
    publish(topic, jsonStr);
}
