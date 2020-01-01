#ifndef HSDMQTT_H
#define HSDMQTT_H

#define MAX_IN_TOPICS        5
#define MQTT_MAX_PACKET_SIZE 256

#include <ArduinoJson.h>
#include <PubSubClient.h>
#ifdef ESP32
#include <WiFi.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#endif

#include "HSDConfig.hpp"

class HSDMqtt {
public:
    HSDMqtt(const HSDConfig& config, MQTT_CALLBACK_SIGNATURE);

    bool        addTopic(const String& topic);
    void        begin();
    inline bool connected() const { return m_pubSubClient.connected(); }
    void        handle();
    inline bool isTopicValid(const String& topic) const { return topic.length() > 0; }
    void        publish(const String& topic, String msg) const;
    void        publish(const String& topic, const JsonObject& json) const;
    bool        reconnect() const; 

private:
    void subscribe(const String& topic) const;

    const HSDConfig&     m_config;
    String               m_inTopics[MAX_IN_TOPICS];
    uint32_t             m_numberOfInTopics;
    mutable PubSubClient m_pubSubClient;
    WiFiClient           m_wifiClient;  
};

#endif // HSDMQTT_H
