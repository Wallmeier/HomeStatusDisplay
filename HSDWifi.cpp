#include "HSDWifi.hpp"

#define SOFT_AP_SSID  (F("StatusDisplay"))
#define SOFT_AP_PSK   (F("statusdisplay"))

HSDWifi::HSDWifi(const HSDConfig& config) :
    m_config(config),
    m_connectFailure(false),
    m_maxConnectRetries(100),
    m_numConnectRetriesDone(0),
    m_retryDelay(500),
    m_millisLastConnectTry(0),
    m_accessPointActive(false),
    m_lastConnectStatus(false),
    m_wasConnected(false)
{  
}

// ---------------------------------------------------------------------------------------------------------------------

void HSDWifi::begin() {
    WiFi.persistent(false);
    WiFi.hostname(m_config.getHost());
}

// ---------------------------------------------------------------------------------------------------------------------

void HSDWifi::handleConnection() {
    bool isConnected(connected());
    if (isConnected != m_lastConnectStatus) {
        if (isConnected) {
            Serial.print(F("WiFi connected with IP "));
            Serial.print(WiFi.localIP());
            Serial.println(F("."));
            m_wasConnected = true;
            m_numConnectRetriesDone = 0;
        } else {
            Serial.println(F("WiFi connection lost."));
        }
        m_lastConnectStatus = isConnected;
    }
  
    if (!isConnected && !m_accessPointActive) {  
        if (m_connectFailure) {
            startAccessPoint();
        } else {
            if ((millis() - m_millisLastConnectTry) >= m_retryDelay) {
                m_millisLastConnectTry = millis(); 
                if (m_numConnectRetriesDone == 0) {
                    Serial.print(F("Starting Wifi connection to "));
                    Serial.print(m_config.getWifiSSID());
                    Serial.println(F("..."));
      
                    WiFi.mode(WIFI_STA);
                    WiFi.begin(m_config.getWifiSSID(), m_config.getWifiPSK());
      
                    m_numConnectRetriesDone++;
                } else if (m_numConnectRetriesDone < m_maxConnectRetries) {
                    m_numConnectRetriesDone++;
                } else {
                    Serial.println(F("Failed to connect WiFi."));

                    // if successfully connected before reboot otherwise start access point
                    if (m_wasConnected) {
                        ESP.restart();
                    } else {
                        m_connectFailure = true;
                    }
                }   
            }   
        }
    }
}

// ---------------------------------------------------------------------------------------------------------------------

void HSDWifi::startAccessPoint() {
    Serial.println(F(""));
    Serial.println(F("Starting access point."));

    WiFi.mode(WIFI_AP);

    if (WiFi.softAP(String(SOFT_AP_SSID).c_str(), String(SOFT_AP_PSK).c_str())) {
        IPAddress ip = WiFi.softAPIP();
        Serial.print(F("AccessPoint SSID is ")); Serial.println(SOFT_AP_SSID); 
        Serial.print(F("IP: ")); Serial.println(ip);

        m_accessPointActive = true;
    } else {
        Serial.println(F("Error starting access point."));
    }
}
