#include "HSDWifi.hpp"
#include "HSDLogger.hpp"

#include <ArduinoOTA.h>

#ifdef ARDUINO_ARCH_ESP32
#include <ESPmDNS.h>
#else
#include <ESP8266mDNS.h>
#endif

#define SOFT_AP_SSID  "StatusDisplay"
#define SOFT_AP_PSK   "statusdisplay"

HSDWifi::HSDWifi(const HSDConfig* config, HSDLeds* leds, HSDWebserver* webserver) :
    m_accessPointActive(false),
    m_config(config),
    m_connectFailure(false),
    m_lastConnectStatus(false),
    m_leds(leds),
    m_maxConnectRetries(100),
    m_millisLastConnectTry(0),
    m_numConnectRetriesDone(0),
    m_retryDelay(500),
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
        WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE); // call is only a workaround for bug in WiFi class for setting hostname
        WiFi.begin(m_config->getWifiSSID().c_str(), m_config->getWifiPSK().c_str(), 0, nullptr, false);
    
#ifdef ESP32    
        if (!WiFi.setHostname(m_config->getHost().c_str()))
#elif defined(ESP8266)
        WiFi.setSleepMode(WIFI_NONE_SLEEP);
        if (!WiFi.hostname(m_config->getHost()))
#endif // ARDUINO_ARCH_ESP32
            Logger.log("Failed to set hostname: %s", m_config->getHost().c_str());
        Logger.log("WiFi settings");
        Logger.log("  • AutoConnect: %s", WiFi.getAutoConnect() ? "true" : "false");
        Logger.log("  • AutoReconnect: %s", WiFi.getAutoReconnect() ? "true" : "false");
#ifdef ESP8266
        Logger.log("  • Hostname: %s", WiFi.hostname().c_str());
        Logger.log("  • PhyMode: %d", WiFi.getPhyMode());
        Logger.log("  • SleepMode: %d", WiFi.getSleepMode());
#elif defined(ESP32)
        Logger.log("  • Hostname: %s", WiFi.getHostname());
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
        m_evOnDhcpTimeout = WiFi.onStationModeDHCPTimeout([=]() {
            Logger.log("DHCP timeout");
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
            if (first || ((millis() - m_millisLastConnectTry) >= static_cast<unsigned long>(m_retryDelay))) {
                first = false;
                m_millisLastConnectTry = millis(); 
                if (m_numConnectRetriesDone == 0) {
                    Logger.log("Starting Wifi connection to %s", m_config->getWifiSSID().c_str());

                    WiFi.reconnect();

                    m_numConnectRetriesDone++;
                } else if (m_numConnectRetriesDone < m_maxConnectRetries) {
                    m_numConnectRetriesDone++;
                } else {
                    Logger.log("Failed to connect WiFi.");

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
    Logger.log("Connected to WiFi '%s' on AP %s with channel %d", ssid.c_str(), bssid.c_str(), channel);
    m_webserver->updateStatusEntry("ssid", ssid);
    m_webserver->updateStatusEntry("bssid", bssid);
    m_webserver->updateStatusEntry("channel", String(channel));

    if (initialConnect) {
        initialConnect = false;

#ifdef ESP32
        ArduinoOTA.setPort(3232);
#elif defined(ESP8266)
        ArduinoOTA.setPort(8266);
#endif
        ArduinoOTA.setHostname(m_config->getHost().c_str());
        ArduinoOTA.begin();
        Logger.log("ArduinoOTA started");

        if (!MDNS.begin(m_config->getHost().c_str())) 
            Logger.log("Failed to start MDNS");
        MDNS.addService("http", "tcp", 80);
        m_leds->setAllOn(LED_COLOR_YELLOW);
    }
}

// ---------------------------------------------------------------------------------------------------------------------

void HSDWifi::onDisconnect(const String& ssid, const String& bssid, uint8_t reason) const {
    Logger.log("Disconnected from WiFi '%s' on AP %s: reason %d",  ssid.c_str(), bssid.c_str(), reason);
    m_leds->setAllOn(LED_COLOR_RED);
}

// ---------------------------------------------------------------------------------------------------------------------

void HSDWifi::onGotIP(const String& ip, const String& netMask, const String& gw) const {
    Logger.log("Got ip address %s, net mask %s, gateway %s", ip.c_str(), netMask.c_str(), gw.c_str());
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
    Logger.log();
    Logger.log("Starting access point.");

    WiFi.mode(WIFI_AP);

    if (WiFi.softAP(String(SOFT_AP_SSID).c_str(), String(SOFT_AP_PSK).c_str())) {
        m_accessPointActive = true;
        Logger.log("AccessPoint SSID is %s", SOFT_AP_SSID); 
        Logger.log("IP: %s", WiFi.softAPIP().toString().c_str());
    } else {
        Logger.log("Error starting access point.");
    }
}
