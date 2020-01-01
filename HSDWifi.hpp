#ifndef HSDWIFI_H
#define HSDWIFI_H

#ifdef ESP32
#include <lwip/ip4_addr.h>
#include <WiFi.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#endif

#include "HSDConfig.hpp"
#include "HSDLeds.hpp"
#include "HSDWebserver.hpp"

class HSDWifi {
public:
    HSDWifi(const HSDConfig& config, HSDLeds& leds, HSDWebserver& webserver);

    void   begin();
    void   handleConnection();

private:
    void   onConnect(const String& ssid, const String& bssid, uint8_t channel) const;
    void   onDisconnect(const String& ssid, const String& bssid, uint8_t reason) const;
    void   onGotIP(const String& ip, const String& netMask, const String& gw) const;
#ifdef ESP32
    String printIP(ip4_addr_t& ip) const;
#endif    
    String printMac(const uint8_t mac[6]) const;
    void   startAccessPoint();

    bool             m_accessPointActive;
    const HSDConfig& m_config;
    bool             m_connectFailure;
#ifdef ESP8266
    WiFiEventHandler m_evOnConnect;
    WiFiEventHandler m_evOnDisconnect;
    WiFiEventHandler m_evOnGotIP;
    WiFiEventHandler m_evOnDhcpTimeout;
#elif defined(ESP32)
    WiFiEventId_t    m_evOnConnect;
    WiFiEventId_t    m_evOnDisconnect;
    WiFiEventId_t    m_evOnGotIP;
#endif
    bool             m_lastConnectStatus;
    HSDLeds&         m_leds;
    int              m_maxConnectRetries;
    unsigned long    m_millisLastConnectTry;
    int              m_numConnectRetriesDone;
    int              m_retryDelay;
    bool             m_wasConnected;
    HSDWebserver&    m_webserver;
};

#endif // HSDWIFI_H
