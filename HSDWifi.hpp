#ifndef HSDWIFI_H
#define HSDWIFI_H

#include <ESP8266WiFi.h>

#include "HSDConfig.hpp"

class HSDWifi {
public:
    HSDWifi(const HSDConfig& config);

    void        begin();
    inline bool connected() const { return (WiFi.status() == WL_CONNECTED); }
    void        handleConnection();

private:
    void startAccessPoint();

    bool             m_accessPointActive;
    const HSDConfig& m_config;
    bool             m_connectFailure;
    bool             m_lastConnectStatus;
    int              m_maxConnectRetries;
    unsigned long    m_millisLastConnectTry;
    int              m_numConnectRetriesDone;
    int              m_retryDelay;
    bool             m_wasConnected;
};

#endif // HSDWIFI_H
