#ifndef HSDMQTT_H
#define HSDMQTT_H

#include <ESP8266WiFi.h>
#include <PubSubClient.h>

#include "HSDConfig.hpp"

#define MAX_IN_TOPICS 5

class HSDMqtt {
public:
    HSDMqtt(const HSDConfig& config, MQTT_CALLBACK_SIGNATURE);

    bool        addTopic(const String& topic);
    void        begin();
    inline bool connected() const { return m_pubSubClient.connected(); }
    void        handle();
    void        publish(String topic, String msg) const;
    bool        reconnect() const; 

private:
    inline bool isTopicValid(const String& topic) const { return topic.length() > 0; }
    void        subscribe(const String& topic) const;

    const HSDConfig&     m_config;
    bool                 m_connectFailure;
    String               m_inTopics[MAX_IN_TOPICS];
    unsigned long        m_millisLastConnectTry;
    uint32_t             m_numberOfInTopics;
    int                  m_numConnectRetriesDone;
    mutable PubSubClient m_pubSubClient;
    WiFiClient           m_wifiClient;  
};

#endif // HSDMQTT_H
