#include "HSDWifi.hpp"

#include <ArduinoOTA.h>

#ifdef ARDUINO_ARCH_ESP32
#include <ESPmDNS.h>
#else
#include <ESP8266mDNS.h>
#endif

#define SOFT_AP_SSID  "StatusDisplay"
#define SOFT_AP_PSK   "statusdisplay"

HSDWifi::HSDWifi(const HSDConfig* config, HSDLeds* leds, HSDWebserver* webserver) :
    m_config(config),
    m_connectFailure(false),
    m_leds(leds),
    m_maxConnectRetries(100),
    m_numConnectRetriesDone(0),
    m_retryDelay(500),
    m_millisLastConnectTry(0),
    m_accessPointActive(false),
    m_lastConnectStatus(false),
    m_wasConnected(false),
    m_webserver(webserver)
{
}

// ---------------------------------------------------------------------------------------------------------------------

void HSDWifi::begin() {
    if (m_config->getWifiSSID().length() > 0) {
        m_leds->setAllOn(LED_COLOR_RED);
        WiFi.setAutoConnect(false);
        WiFi.persistent(false);
        WiFi.mode(WIFI_STA);
        WiFi.begin(m_config->getWifiSSID().c_str(), m_config->getWifiPSK().c_str(), 0, nullptr, false);
    
#ifdef ESP32    
        if (!WiFi.setHostname(m_config->getHost().c_str()))
#elif defined(ESP8266)
        WiFi.setSleepMode(WIFI_NONE_SLEEP);
        if (!WiFi.hostname(m_config->getHost()))
#endif // ARDUINO_ARCH_ESP32
            Serial.printf("Failed to set hostname: %s\n", m_config->getHost().c_str());
        Serial.println("WiFi settings");
        Serial.printf("  • AutoConnect: %s\n", WiFi.getAutoConnect() ? "true" : "false");
        Serial.printf("  • AutoReconnect: %s\n", WiFi.getAutoReconnect() ? "true" : "false");
#ifdef ESP8266
        Serial.printf("  • Hostname: %s\n", WiFi.hostname().c_str());
        Serial.printf("  • PhyMode: %d\n", WiFi.getPhyMode());
        Serial.printf("  • SleepMode: %d\n", WiFi.getSleepMode());
#elif defined(ESP32)
        Serial.printf("  • Hostname: %s\n", WiFi.getHostname());
#endif    
#ifdef ESP8266
        m_evOnConnect = WiFi.onStationModeConnected([=](const WiFiEventStationModeConnected& event) {
            onConnect(event.ssid, printMac(event.bssid), event.channel);
        });
        m_evOnDisconnect = WiFi.onStationModeDisconnected([=](const WiFiEventStationModeDisconnected& event) {
            onDisconnect(event.ssid, printMac(event.bssid), event.reason);
        });
        m_evOnGotIP = WiFi.onStationModeGotIP([=](const WiFiEventStationModeGotIP& event) {
            onGotIP(event.ip.toString(), event.mask.toString(), event.gw.toString());
        });
        m_evOnDhcpTimeout = WiFi.onStationModeDHCPTimeout([]() {
            Serial.println("DHCP timeout");
        });
#elif defined(ESP32)    
        m_evOnConnect = WiFi.onEvent([=](WiFiEvent_t event, WiFiEventInfo_t info) {
            onConnect(String(reinterpret_cast<const char*>(info.connected.ssid)), printMac(info.connected.bssid), info.connected.channel);
        }, WiFiEvent_t::SYSTEM_EVENT_STA_CONNECTED);
        m_evOnDisconnect = WiFi.onEvent([=](WiFiEvent_t event, WiFiEventInfo_t info) {
            onDisconnect(String(reinterpret_cast<const char*>(info.disconnected.ssid)), printMac(info.disconnected.bssid), info.disconnected.reason);
        }, WiFiEvent_t::SYSTEM_EVENT_STA_DISCONNECTED);
        m_evOnGotIP = WiFi.onEvent([=](WiFiEvent_t event, WiFiEventInfo_t info) {
            onGotIP(printIP(info.got_ip.ip_info.ip), printIP(info.got_ip.ip_info.netmask), printIP(info.got_ip.ip_info.gw));
        }, WiFiEvent_t::SYSTEM_EVENT_STA_GOT_IP);
#endif    
        handleConnection();
    } else {
        startAccessPoint();
    }
}

// ---------------------------------------------------------------------------------------------------------------------

void HSDWifi::handleConnection() {
    static bool first = true;
    bool isConnected(WiFi.isConnected());
    if (isConnected != m_lastConnectStatus) {
        if (isConnected) {
            m_wasConnected = true;
            m_numConnectRetriesDone = 0;
        }
        m_lastConnectStatus = isConnected;
    }

    if (!isConnected && !m_accessPointActive) {  
        if (m_connectFailure) {
            startAccessPoint();
        } else {
            if (first || ((millis() - m_millisLastConnectTry) >= m_retryDelay)) {
                first = false;
                m_millisLastConnectTry = millis(); 
                if (m_numConnectRetriesDone == 0) {
                    Serial.print("Starting Wifi connection to ");
                    Serial.println(m_config->getWifiSSID());

                    WiFi.reconnect();

                    m_numConnectRetriesDone++;
                } else if (m_numConnectRetriesDone < m_maxConnectRetries) {
                    m_numConnectRetriesDone++;
                } else {
                    Serial.println("Failed to connect WiFi.");

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

void HSDWifi::onConnect(const String& ssid, const String& bssid, uint8_t channel) const {
    static bool initialConnect = true;
    Serial.printf("Connected to WiFi '%s' on AP %s with channel %d\n", ssid.c_str(), bssid.c_str(), channel);
    m_webserver->updateStatusEntry("ssid", ssid);
    m_webserver->updateStatusEntry("bssid", bssid);
    m_webserver->updateStatusEntry("channel", String(channel));

    if (initialConnect) {
        initialConnect = false;
        ArduinoOTA.begin();
        MDNS.addService("http", "tcp", 80);
        m_leds->setAllOn(LED_COLOR_YELLOW);
    }
}

// ---------------------------------------------------------------------------------------------------------------------

void HSDWifi::onDisconnect(const String& ssid, const String& bssid, uint8_t reason) const {
    Serial.printf("Disconnected from WiFi '%s' on AP %s: reason %d\n",  ssid.c_str(), bssid.c_str(), reason);
    m_leds->setAllOn(LED_COLOR_RED);
}

// ---------------------------------------------------------------------------------------------------------------------

void HSDWifi::onGotIP(const String& ip, const String& netMask, const String& gw) const {
    Serial.printf("Got ip address %s, net mask %s, gateway %s\n", ip.c_str(), netMask.c_str(), gw.c_str());
    m_webserver->updateStatusEntry("gateway", gw);
    m_webserver->updateStatusEntry("ip", ip);
    m_webserver->updateStatusEntry("subnetMask", netMask);
}

// ---------------------------------------------------------------------------------------------------------------------
#ifdef ESP32
String HSDWifi::printIP(ip4_addr_t& ip) const {
    char res[16];
    snprintf(res, 16, "%u.%u.%u.%u", ip4_addr1_16(&ip), ip4_addr2_16(&ip), ip4_addr3_16(&ip), ip4_addr4_16(&ip));
    return String(res);
}
#endif
// ---------------------------------------------------------------------------------------------------------------------

String HSDWifi::printMac(const uint8_t mac[6]) const {
    char res[20];
    snprintf(res, 20, "%02X:%02X:%02X:%02X:%02X:%02X", (mac[0] & 0xff), (mac[1] & 0xff), (mac[2] & 0xff), 
                                                       (mac[3] & 0xff), (mac[4] & 0xff), (mac[5] & 0xff));
    return String(res);
}

// ---------------------------------------------------------------------------------------------------------------------

void HSDWifi::startAccessPoint() {
    Serial.println("");
    Serial.println("Starting access point.");

    WiFi.mode(WIFI_AP);

    if (WiFi.softAP(String(SOFT_AP_SSID).c_str(), String(SOFT_AP_PSK).c_str())) {
        m_accessPointActive = true;
        Serial.print("AccessPoint SSID is "); Serial.println(SOFT_AP_SSID); 
        Serial.print("IP: "); Serial.println(WiFi.softAPIP());
    } else {
        Serial.println("Error starting access point.");
    }
}
